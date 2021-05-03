#include "tech-core/subsystem/terrain.hpp"
#include "vulkanutils.hpp"

#include "tech-core/utilities/profiler.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <iostream>

// The allocation size in bytes for terrain mesh buffers
#define BUFFER_ALLOCATE_SIZES 3145728

// #define TC_SORT_TRIANGLES

namespace Engine::Subsystem {
namespace _E = Engine;

// TerrainSubsystem
const SubsystemID<TerrainSubsystem> TerrainSubsystem::ID;

TerrainSubsystem::TerrainSubsystem() {}

uint32_t TerrainSubsystem::addSegment(
    const std::vector<TileVertex> &vertices,
    const std::vector<uint16_t> &indices,
    uint32_t textureArrayId,
    glm::vec3 minCorner,
    glm::vec3 maxCorner,
    bool isTranslucent
) {
    ProfilerSection profile("addSegment");

    vk::DeviceSize vertexSize = sizeof(TileVertex) * vertices.size();
    vk::DeviceSize indexSize = sizeof(uint16_t) * indices.size();

    // Note: The indices should be aligned to 2 bytes, as long as the vertices at an even size we are ok.
    vk::DeviceSize size = vertexSize + indexSize;
    TerrainSegment &segment = allocate(size, isTranslucent);
    
    segment.indexOffset = segment.offset + vertexSize;
    segment.indexType = vk::IndexType::eUint16;
    segment.indexCount = indices.size();
    segment.textureArrayId = textureArrayId;
    segment.minCorner = minCorner;
    segment.maxCorner = maxCorner;

    // Push up to the GPU buffer
    auto task = taskManager->createTask();
    if (isTranslucent) {
        // Copy directly since it is available on the CPU
        segment.buffer->copyIn(vertices.data(), segment.offset, vertexSize);
        segment.buffer->copyIn(indices.data(), segment.indexOffset, indexSize);
        // TODO: Probably wont be doing this once we sort them
        segment.buffer->flushRange(segment.offset, size);
    } else {

        // TODO: more efficient copying
        std::shared_ptr<Buffer> tempBuffer = bufferManager->aquireStaging(size);
        
        tempBuffer->copyIn(vertices.data(), vertexSize);
        tempBuffer->copyIn(indices.data(), vertexSize, indexSize);

        task->execute([&segment, staging = tempBuffer.get(), size](vk::CommandBuffer commandBuffer) {
            staging->transfer(commandBuffer, *segment.buffer, 0, segment.offset, size);
        });
        task->executeWhenComplete([this, tempBuffer] () mutable {
            tempBuffer.reset();
        });
    }

    task->addMemoryBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::AccessFlagBits::eTransferWrite,
        vk::PipelineStageFlagBits::eVertexInput,
        vk::AccessFlagBits::eVertexAttributeRead
    );

    taskManager->submitTask(std::move(task));
    requireSort = true;

    return segment.id;
}

uint32_t TerrainSubsystem::updateSegment(
    uint32_t id,
    const std::vector<TileVertex> &vertices,
    const std::vector<uint16_t> &indices,
    uint32_t textureArrayId,
    glm::vec3 minCorner,
    glm::vec3 maxCorner,
    bool isTranslucent
) {
    ProfilerSection profile("updateSegment");

    SegmentMap *segmentMap;
    if (isTranslucent) {
        segmentMap = &transparentSegments;
    } else {
        segmentMap = &segments;
    }

    auto it = segmentMap->find(id);
    if (it == segmentMap->end()) {
        return addSegment(vertices, indices, textureArrayId, minCorner, maxCorner, isTranslucent);
    }

    TerrainSegment *segment = &it->second;

    vk::DeviceSize vertexSize = sizeof(TileVertex) * vertices.size();
    vk::DeviceSize indexSize = sizeof(uint16_t) * indices.size();

    // Note: The indices should be aligned to 2 bytes, as long as the vertices at an even size we are ok.
    vk::DeviceSize size = vertexSize + indexSize;

    if (segment->size < size) {
        // Need to resize
        free(segment);
        segment = &allocate(size, isTranslucent);
    }
    // TODO: Space reclamation

    segment->indexOffset = segment->offset + vertexSize;
    segment->indexType = vk::IndexType::eUint16;
    segment->indexCount = indices.size();
    segment->textureArrayId = textureArrayId;
    segment->translucent = isTranslucent;
    segment->minCorner = minCorner;
    segment->maxCorner = maxCorner;

    // Push up to the GPU buffer
    auto task = taskManager->createTask();
    if (isTranslucent) {
        // Copy directly since it is available on the CPU
        segment->buffer->copyIn(vertices.data(), segment->offset, vertexSize);
        segment->buffer->copyIn(indices.data(), segment->indexOffset, indexSize);
        // TODO: Probably wont be doing this once we sort them
        segment->buffer->flushRange(segment->offset, size);
    } else {
        // TODO: more efficient copying
        std::shared_ptr<Buffer> tempBuffer = bufferManager->aquireStaging(size);
        tempBuffer->copyIn(vertices.data(), vertexSize);
        tempBuffer->copyIn(indices.data(), vertexSize, indexSize);

        task->execute([&segment, staging = tempBuffer.get(), size](vk::CommandBuffer commandBuffer) {
            staging->transfer(commandBuffer, *segment->buffer, 0, segment->offset, size);
        });

        task->executeWhenComplete([this, tempBuffer] () mutable {
            tempBuffer.reset();
        });
    }

    task->addMemoryBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::AccessFlagBits::eTransferWrite,
        vk::PipelineStageFlagBits::eVertexInput,
        vk::AccessFlagBits::eVertexAttributeRead
    );

    taskManager->submitTask(std::move(task));
    requireSort = true;

    return segment->id;
}

void TerrainSubsystem::removeSegment(uint32_t id) {
    auto it = segments.find(id);
    if (it != segments.end()) {
        free(&it->second);
    } else {
        it = transparentSegments.find(id);
        if (it != transparentSegments.end()) {
            free(&it->second);
        }
    }
}

TerrainSegment &TerrainSubsystem::allocate(vk::DeviceSize size, bool transparent) {
    ProfilerSection profile("allocate");
    // Find a free buffer or allocate a new one
    _E::DivisibleBuffer *targetBuffer = nullptr;
    vk::DeviceSize offset;

    BufferVec *buffers;
    SegmentMap *segmentMap;
    if (transparent) {
        buffers = &transparentMeshBuffers;
        segmentMap = &transparentSegments;
    } else {
        buffers = &meshBuffers;
        segmentMap = &segments;
    }

    for (auto &buffer : *buffers) {
        offset = buffer->allocateSection(size);
        if (offset != ALLOCATION_FAILED) {
            targetBuffer = buffer.get();
            break;
        }
    }

    if (targetBuffer == nullptr) {
        auto buffer = bufferManager->aquireDivisible(
            BUFFER_ALLOCATE_SIZES,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
            transparent ? vk::MemoryUsage::eCPUToGPU : vk::MemoryUsage::eGPUOnly
        );
        targetBuffer = buffer.get();
        offset = buffer->allocateSection(size);

        if (offset == ALLOCATION_FAILED) {
            // This is very unexpected
            throw std::runtime_error("Terrain mesh allocation failed on fresh mesh buffer");
        }

        buffers->push_back(std::move(buffer));
    }

    // Allocate the section info    
    uint32_t id = nextId++;
    (*segmentMap)[id] = {
        id,
        targetBuffer,
        offset,
        size,
        0, // Index offset
        0, // Index count
        vk::IndexType::eNoneNV, // Index type
        0, // texture array Id
        transparent // translucent
    };

    // std::cout << "New segment " << id << std::endl;

    return (*segmentMap)[id];
}

void TerrainSubsystem::free(TerrainSegment *segment) {
    if (!segment) {
        return;
    }

    SegmentMap *segmentMap;
    if (segment->translucent) {
        segmentMap = &transparentSegments;
    } else {
        segmentMap = &segments;
    }

    segment->buffer->freeSection(
        segment->offset,
        segment->size
    );

    // std::cout << "Removed segment " << segment->id << std::endl;
    segmentMap->erase(segment->id);
    requireSort = true;
    // segment pointer not valid beyond here
}

void TerrainSubsystem::initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) {
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {{
        { // Camera binding
            0, // binding
            vk::DescriptorType::eUniformBuffer,
            1, // count
            vk::ShaderStageFlagBits::eVertex
        }
    }};
    
    descriptorLayout = device.createDescriptorSetLayout({
        {}, vkUseArray(bindings)
    });

    bufferManager = &engine.getBufferManager();
    taskManager = &engine.getTaskManager();
    textureManager = &engine.getTextureManager();

    terrainSamplerId = engine.getMaterialManager().createSampler({
        vk::Filter::eNearest,
        vk::Filter::eNearest,
        vk::SamplerMipmapMode::eNearest,
        false
    });
    terrainSampler = engine.getMaterialManager().getSamplerById(terrainSamplerId);
    globalLight = engine.getSubsystem(LightSubsystem::ID);
    this->engine = &engine;
}

void TerrainSubsystem::initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) {
    // Descriptor pool for allocating the descriptors
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {
            vk::DescriptorType::eUniformBuffer,
            swapChainImages
        }
    }};

    descriptorPool = device.createDescriptorPool({
        {},
        swapChainImages,
        vkUseArray(poolSizes)
    });

    // Descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(swapChainImages, descriptorLayout);

    descriptorSets = device.allocateDescriptorSets({
        descriptorPool,
        vkUseArray(layouts)
    });

    // Assign buffers to DS'
    for (uint32_t imageIndex = 0; imageIndex < swapChainImages; ++imageIndex) {
        auto cameraUbo = engine.getCameraDBI(imageIndex);

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {{
            { // Camera UBO
                descriptorSets[imageIndex],
                0, // Binding
                0, // Array element
                1, // Count
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &cameraUbo
            }
        }};

        device.updateDescriptorSets(descriptorWrites, {});
    }

    pipelineNormal = engine.createPipeline()
        .withVertexShader("assets/shaders/terrain-vert.spv")
        .withFragmentShader("assets/shaders/terrain-frag-solid.spv")
        .withGeometryType(PipelineGeometryType::Polygons)
        .withVertexAttributeDescriptions(TileVertex::getAttributeDescriptions())
        .withVertexBindingDescription(TileVertex::getBindingDescription())
        .withDescriptorSet(descriptorLayout)
        .withDescriptorSet(globalLight->layout())
        .withDescriptorSet(engine.getTextureManager().getLayout())
        .build();
    pipelineTranslucent = engine.createPipeline()
        .withVertexShader("assets/shaders/terrain-vert.spv")
        .withFragmentShader("assets/shaders/terrain-frag-trans.spv")
        .withGeometryType(PipelineGeometryType::Polygons)
        .withVertexAttributeDescriptions(TileVertex::getAttributeDescriptions())
        .withVertexBindingDescription(TileVertex::getBindingDescription())
        .withDescriptorSet(descriptorLayout)
        .withDescriptorSet(globalLight->layout())
        .withDescriptorSet(engine.getTextureManager().getLayout())
        .withAlpha()
        .withoutDepthWrite()
        .withoutFaceCulling()
        .build();
}

void TerrainSubsystem::cleanupResources(vk::Device device, _E::RenderEngine &engine) {
    meshBuffers.clear();
    transparentMeshBuffers.clear();
    device.destroyDescriptorSetLayout(descriptorLayout);
}

void TerrainSubsystem::cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) {
    pipelineNormal.reset();
    pipelineTranslucent.reset();
    device.destroyDescriptorPool(descriptorPool);
}

void TerrainSubsystem::prepareFrame(uint32_t activeImage) {
    sortTriangles(requireSort);
    requireSort = false;
}

void TerrainSubsystem::writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) {
    ProfilerSection profile("writeFrameCommands");

    Profiler::enter("render solid");
    pipelineNormal->bind(commandBuffer);

    std::array<vk::DescriptorSet, 2> globalDescriptors = {
        descriptorSets[activeImage],
        globalLight->descriptor(activeImage)
    };
    pipelineNormal->bindDescriptorSets(commandBuffer, 0, vkUseArray(globalDescriptors), 0, nullptr);

    auto &frustum = this->engine->getCamera()->getFrustum();
    
    for (auto &segmentPair : segments) {
        auto &segment = segmentPair.second;
        if (segment.indexCount == 0 || segment.translucent) {
            continue;
        }

        if (!frustum.intersects(segment.minCorner, segment.maxCorner)) {
            continue;
        }

        // Texture binding
        auto binding = textureManager->getBinding(segment.textureArrayId, terrainSamplerId, terrainSampler);
        pipelineNormal->bindDescriptorSets(commandBuffer, 2, 1, &binding, 0, nullptr);

        commandBuffer.bindVertexBuffers(0, 1, segment.buffer->bufferArray(), &segment.offset);
        commandBuffer.bindIndexBuffer(segment.buffer->buffer(), segment.indexOffset, segment.indexType);
        commandBuffer.drawIndexed(segment.indexCount, 1, 0, 0, 0);
    }
    Profiler::leave();

    Profiler::enter("render translucent");
    pipelineTranslucent->bind(commandBuffer);

    pipelineTranslucent->bindDescriptorSets(commandBuffer, 0, vkUseArray(globalDescriptors), 0, nullptr);
    
    for (auto segment : orderedTransparentSegments) {
        if (segment->indexCount == 0) {
            continue;
        }

        if (!frustum.intersects(segment->minCorner, segment->maxCorner)) {
            continue;
        }

        // Texture binding
        auto binding = textureManager->getBinding(segment->textureArrayId, terrainSamplerId, terrainSampler);
        pipelineTranslucent->bindDescriptorSets(commandBuffer, 2, 1, &binding, 0, nullptr);

        commandBuffer.bindVertexBuffers(0, 1, segment->buffer->bufferArray(), &segment->offset);
        commandBuffer.bindIndexBuffer(segment->buffer->buffer(), segment->indexOffset, segment->indexType);
        commandBuffer.drawIndexed(segment->indexCount, 1, 0, 0, 0);
    }
    Profiler::leave();
}

/**
 * Sorts translucent triangles to be Back to Front order.
 * This will ensure that translucent things render correctly
 */
void TerrainSubsystem::sortTriangles(bool force) {
    ProfilerSection profile("sortTriangles");

    auto cam = engine->getCamera();
    if (!cam) {
        return;
    }

    auto pos = cam->getPosition();

    if (!force) {
        auto dist = glm::length2(pos - lastCamPos);
        if (dist < sortThreshold) {
            return;
        }

        lastCamPos = pos;
    }

    // Re-order both segments, and the triangles within based on the player position.

    // First: order segments so they render in back to front order. first item is furthest
    orderedTransparentSegments.clear();
    orderedTransparentSegments.reserve(transparentSegments.size());
    std::vector<float> distVec;

    distVec.reserve(transparentSegments.size());
    
    for (auto &pair : transparentSegments) {
        auto &segment = pair.second;
        auto center = (segment.minCorner + segment.maxCorner);
        center *= 0.5;

        auto dist = glm::length2(pos - center);

        if (distVec.size() == 0) {
            orderedTransparentSegments.push_back(&segment);
            distVec.push_back(dist);
        } else {
            // Insertion sort using binary search
            int low = 0;
            int high = static_cast<int>(distVec.size()) - 1;

            while (low <= high) {
                auto mid = (low + high) / 2;
                if (dist > distVec[mid]) {
                    high = mid - 1;
                } else {
                    low = mid + 1;
                }
            }

            distVec.insert(distVec.begin() + low, dist);
            orderedTransparentSegments.insert(orderedTransparentSegments.begin() + low, &segment);
        }

        #ifdef TC_SORT_TRIANGLES
        // Second: order triangles within segments
        void *data;
        segment.buffer->map(&data);

        TileVertex *vertexData = reinterpret_cast<TileVertex*>(data + segment.offset);
        uint16_t *indexData = reinterpret_cast<uint16_t*>(data + segment.indexOffset);

        // Work out the distance to each triangle
        auto triangleCount = segment.indexCount / 3;
        std::vector<float> triangleDistances;
        triangleDistances.reserve(triangleCount);

        for (auto offset = 0, triangle = 0; offset <= segment.indexCount - 3; offset += 3, ++triangle) {
            // Vertex data of triangle
            auto index1 = indexData[offset+0];
            auto index2 = indexData[offset+1];
            auto index3 = indexData[offset+2];
            auto &v1 = vertexData[index1];
            auto &v2 = vertexData[index2];
            auto &v3 = vertexData[index3];

            auto vCenter = v1.pos;
            vCenter += v2.pos;
            vCenter += v3.pos;
            vCenter /= 3;

            float distX = pos.x - vCenter.x;
            float distY = pos.y - vCenter.y;
            float distZ = pos.z - vCenter.z;

            float dist = (distX * distX) + (distY * distY) + (distZ * distZ);

            if (triangleDistances.size() == 0) {
                triangleDistances.push_back(dist);
            } else {
                // Insertion sort using binary search
                int low = 0;
                int high = static_cast<int>(triangleDistances.size()) - 1;

                // Shortcut, dont do insertion sort if its just smaller than everything else
                // There would be no need to move any indices
                if (dist < triangleDistances[triangleDistances.size()-1]) {
                    triangleDistances.push_back(dist);
                    continue;
                }

                while (low <= high) {
                    auto mid = (low + high) / 2;
                    if (dist > triangleDistances[mid]) {
                        high = mid - 1;
                    } else {
                        low = mid + 1;
                    }
                }

                triangleDistances.insert(triangleDistances.begin() + low, dist);

                // Either will be the current triangle or before
                if (low != triangle) {
                    auto fromOffset = low * 3;
                    auto toOffset = (low + 1) * 3;
                    auto moveSize = (triangle - low) * 3 * sizeof(uint16_t);

                    // Shift everything from low to make room from the stuff
                    std::memmove(indexData + toOffset, indexData + fromOffset, moveSize);
                    indexData[fromOffset+0] = index1;
                    indexData[fromOffset+1] = index2;
                    indexData[fromOffset+2] = index3;

                    // TODO: We might be able to make this cheaper if we do the moves afterwards.
                    // The idea is, we do all the sorting on a cheaper memory container, then
                    // knowing where everything needs to go, we move once to their final location.
                }
            }
        }

        segment.buffer->unmap();
        segment.buffer->flushRange(segment.offset, segment.size);

        #endif
    }

    auto task = taskManager->createTask();
    task->addMemoryBarrier(
        vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eHost,
        vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eHostWrite,
        vk::PipelineStageFlagBits::eVertexInput,
        vk::AccessFlagBits::eVertexAttributeRead
    );

    taskManager->submitTask(std::move(task));


}

}