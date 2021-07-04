#include <vulkanutils.hpp>
#include "render_planner.hpp"
#include "tech-core/scene/components/mesh_renderer.hpp"
#include "tech-core/scene/components/light.hpp"
#include "tech-core/buffer.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/device.hpp"
#include "components/planner_data.hpp"
#include "tech-core/mesh.hpp"
#include "tech-core/texture/manager.hpp"
#include "tech-core/material/material.hpp"
#include "tech-core/material/manager.hpp"
#include "internal/packaged/builtin_standard_frag_glsl.h"
#include "internal/packaged/builtin_standard_vert_glsl.h"
#include "bindings.hpp"
#include <iostream>

namespace Engine::Internal {

const Subsystem::SubsystemID<RenderPlanner> RenderPlanner::ID;

void RenderPlanner::addEntity(Entity *entity) {
    // An internal component used for storing data against the entity
    if (!entity->has<PlannerData>()) {
        ignoreComponentUpdates = true;
        entity->add<PlannerData>();
        ignoreComponentUpdates = false;
    }

    if (entity->has<MeshRenderer>()) {
        addToRender(entity);
    }
    if (entity->has<Light>()) {
        addLight(entity);
    }
}

void RenderPlanner::removeEntity(Entity *entity) {
    if (entity->has<MeshRenderer>()) {
        removeFromRender(entity);
    }
    if (entity->has<Light>()) {
        removeLight(entity);
    }
}

void RenderPlanner::updateEntity(Entity *entity, EntityUpdateType update) {
    if (update == EntityUpdateType::Transform) {
        updateTransforms(entity, true);
        updateEntityUniform(entity);
        entity->forEachChild(
            true, [this](Entity *entity) {
                if (entity->get<PlannerData>().render.buffer) {
                    updateEntityUniform(entity);
                }
            }
        );
    } else if (update == EntityUpdateType::Light) {
        if (entity->has<Light>()) {
            updateLightUniform(entity);
        }
    } else if (update == EntityUpdateType::ComponentAdd && !ignoreComponentUpdates) {
        if (entity->has<MeshRenderer>() && !renderableEntities.contains(entity)) {
            addToRender(entity);
        }
        if (entity->has<Light>() && !lightEntities.contains(entity)) {
            addLight(entity);
        }
    } else if (update == EntityUpdateType::ComponentRemove && !ignoreComponentUpdates) {
        if (!entity->has<MeshRenderer>() && renderableEntities.contains(entity)) {
            removeFromRender(entity);
        }
        if (!entity->has<Light>() && lightEntities.contains(entity)) {
            removeLight(entity);
        }
    }
}

void RenderPlanner::addToRender(Entity *entity) {
    renderableEntities.insert(entity);
    auto &data = entity->get<PlannerData>();

    auto pair = allocateEntityUniform();
    data.render.buffer = pair.first;
    data.render.uniformOffset = pair.second;

    updateEntity(entity, EntityUpdateType::Transform);
}

void RenderPlanner::removeFromRender(Entity *entity) {
    renderableEntities.erase(entity);

    auto &data = entity->get<PlannerData>();
    if (data.render.buffer) {
        data.render.buffer->buffer->freeSection(data.render.uniformOffset, uboBufferAlignment);
        data.render.buffer = nullptr;
    }
}

EntityBuffer &RenderPlanner::newEntityBuffer() {
    auto uboBuffer = engine->getBufferManager().aquireDivisible(
        uboBufferMaxSize,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryUsage::eCPUToGPU
    );

    auto descriptorSets = device.allocateDescriptorSets(
        {
            objectDSPool,
            1,
            &objectDSL
        }
    );

    auto objectDS = descriptorSets[0];

    // Assign buffer to DS
    vk::DescriptorBufferInfo bufferInfo(
        uboBuffer->buffer(),
        0,
        sizeof(EntityUBO)
    );

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {{
        { // Object UBO
            objectDS,
            Internal::StandardBindings::EntityUniform,
            0, // Array element
            1, // Count
            vk::DescriptorType::eUniformBufferDynamic,
            nullptr,
            &bufferInfo
        }
    }};

    device.updateDescriptorSets(descriptorWrites, {});

    auto &buffer = entityBuffers.emplace_back();
    buffer.id = entityBuffers.size() - 1;
    buffer.buffer = std::move(uboBuffer);
    buffer.set = objectDS;

    return buffer;
}

std::pair<EntityBuffer *, uint32_t> RenderPlanner::allocateEntityUniform() {
    // Find the next available slot

    // TODO: Need a way to have the allocator consider the alignment requirements
    for (auto &buffer : entityBuffers) {
        auto offset = buffer.buffer->allocateSection(uboBufferAlignment);
        if (offset != ALLOCATION_FAILED) {
            return { &buffer, offset };
        }
    }

    auto &buffer = newEntityBuffer();
    auto offset = buffer.buffer->allocateSection(uboBufferAlignment);
    // Sanity check
    if (offset == ALLOCATION_FAILED) {
        throw std::runtime_error("Out of object slots");
    }

    return { &buffer, offset };
}

void RenderPlanner::initialiseResources(
    vk::Device device, vk::PhysicalDevice physicalDevice, RenderEngine &engine
) {

    // Object shader layout contains the camera UBO
    vk::DescriptorSetLayoutBinding cameraBinding {
        Internal::StandardBindings::CameraUniform,
        vk::DescriptorType::eUniformBuffer,
        1, // count
        vk::ShaderStageFlagBits::eVertex
    };

    cameraAndModelDSL = device.createDescriptorSetLayout({{}, 1, &cameraBinding });

    // Holds the object information
    vk::DescriptorSetLayoutBinding objectBinding {
        Internal::StandardBindings::EntityUniform,
        vk::DescriptorType::eUniformBufferDynamic,
        1, // count
        vk::ShaderStageFlagBits::eVertex
    };

    objectDSL = device.createDescriptorSetLayout({{}, 1, &objectBinding });

    // Holds the light information
    vk::DescriptorSetLayoutBinding lightBinding {
        Internal::StandardBindings::LightUniform,
        vk::DescriptorType::eUniformBufferDynamic,
        1, // count
        vk::ShaderStageFlagBits::eFragment
    };

    lightDSL = device.createDescriptorSetLayout({{}, 1, &lightBinding });

    size_t minimumAlignment = physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;
    uboBufferAlignment = sizeof(EntityUBO);

    uboBufferAlignment = (uboBufferAlignment + minimumAlignment - 1) & ~(minimumAlignment - 1);
    uboBufferMaxSize = physicalDevice.getProperties().limits.maxUniformBufferRange;

    this->device = device;
    this->engine = &engine;

    defaultMaterial = engine.getMaterialManager().getDefault();
}

void RenderPlanner::initialiseSwapChainResources(
    vk::Device device, RenderEngine &engine, uint32_t swapChainImages
) {
    // Descriptor pool for allocating the descriptors
    vk::DescriptorPoolSize cameraBinding = {
        vk::DescriptorType::eUniformBuffer,
        swapChainImages
    };

    descriptorPool = device.createDescriptorPool(
        {
            {},
            swapChainImages,
            1, &cameraBinding
        }
    );

    // Descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(swapChainImages, cameraAndModelDSL);

    cameraAndModelDS = device.allocateDescriptorSets(
        {
            descriptorPool,
            vkUseArray(layouts)
        }
    );

    // Assign buffers to DS'
    for (uint32_t imageIndex = 0; imageIndex < swapChainImages; ++imageIndex) {
        auto cameraUbo = engine.getCameraDBI(imageIndex);

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
            vk::WriteDescriptorSet(
                cameraAndModelDS[imageIndex],
                Internal::StandardBindings::CameraUniform,
                0, // Array element
                1, // Count
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &cameraUbo
            )
        };

        device.updateDescriptorSets(descriptorWrites, {});
    }

    // Descriptor pool for allocating the descriptors
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {
            vk::DescriptorType::eUniformBufferDynamic,
            2000
        }
    }};

    objectDSPool = device.createDescriptorPool(
        {
            {},
            2000,
            vkUseArray(poolSizes)
        }
    );

    auto builder = engine.createPipeline()
        .withVertexShader(BUILTIN_STANDARD_VERT_GLSL, BUILTIN_STANDARD_VERT_GLSL_SIZE)
        .withFragmentShader(BUILTIN_STANDARD_FRAG_GLSL, BUILTIN_STANDARD_FRAG_GLSL_SIZE)
        .bindCamera(0, Internal::StandardBindings::CameraUniform)
        .bindUniformBufferDynamic(1, Internal::StandardBindings::EntityUniform)
        .bindUniformBufferDynamic(2, Internal::StandardBindings::LightUniform)
        .bindMaterial(3, Internal::StandardBindings::AlbedoTexture, MaterialBindPoint::Albedo)
        .bindMaterial(4, Internal::StandardBindings::NormalTexture, MaterialBindPoint::Normal)
        .withVertexBindingDescription(Vertex::getBindingDescription())
        .withVertexAttributeDescriptions(Vertex::getAttributeDescriptions());

    pipelineNormal = builder.build();

    // Re-allocate descriptor sets
    if (!entityBuffers.empty()) {

        std::vector<vk::DescriptorSetLayout> entityLayouts(entityBuffers.size(), objectDSL);

        auto sets = device.allocateDescriptorSets(
            {
                objectDSPool,
                vkUseArray(entityLayouts)
            }
        );

        // Assign buffers to DS'
        for (uint32_t bufferIndex = 0; bufferIndex < entityBuffers.size(); ++bufferIndex) {
            auto buf = entityBuffers[bufferIndex].buffer.get();

            vk::DescriptorBufferInfo objectUbo(
                buf->buffer(),
                0,
                sizeof(EntityUBO)
            );

            std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
                vk::WriteDescriptorSet(
                    sets[bufferIndex],
                    Internal::StandardBindings::EntityUniform,
                    0, // Array element
                    1, // Count
                    vk::DescriptorType::eUniformBufferDynamic,
                    nullptr,
                    &objectUbo
                )
            };

            device.updateDescriptorSets(descriptorWrites, {});
            entityBuffers[bufferIndex].set = sets[bufferIndex];
        }
    }

    if (!lightBuffers.empty()) {
        std::vector<vk::DescriptorSetLayout> lightLayouts(lightBuffers.size(), lightDSL);

        auto sets = device.allocateDescriptorSets(
            {
                objectDSPool,
                vkUseArray(lightLayouts)
            }
        );

        // Assign buffers to DS'
        for (uint32_t bufferIndex = 0; bufferIndex < lightBuffers.size(); ++bufferIndex) {
            auto buf = lightBuffers[bufferIndex].buffer.get();

            vk::DescriptorBufferInfo objectUbo(
                buf->buffer(),
                0,
                sizeof(LightUBO)
            );

            std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
                vk::WriteDescriptorSet(
                    sets[bufferIndex],
                    Internal::StandardBindings::LightUniform,
                    0, // Array element
                    1, // Count
                    vk::DescriptorType::eUniformBufferDynamic,
                    nullptr,
                    &objectUbo
                )
            };

            device.updateDescriptorSets(descriptorWrites, {});
            lightBuffers[bufferIndex].set = sets[bufferIndex];
        }
    }
}

void RenderPlanner::cleanupResources(vk::Device device, RenderEngine &engine) {
    renderableEntities.clear();
    entityBuffers.clear();
    lightEntities.clear();
    lightBuffers.clear();
    device.destroyDescriptorSetLayout(cameraAndModelDSL);
    device.destroyDescriptorSetLayout(objectDSL);
    device.destroyDescriptorSetLayout(lightDSL);
}

void RenderPlanner::cleanupSwapChainResources(vk::Device device, RenderEngine &engine) {

    pipelineNormal.reset();
    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorPool(objectDSPool);
}

void RenderPlanner::writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) {
    const Mesh *lastMesh = nullptr;

    Pipeline *pipeline = pipelineNormal.get();

    pipeline->bind(commandBuffer);

    // Bind camera
    std::array<vk::DescriptorSet, 1> globalDescriptors = {
        cameraAndModelDS[activeImage],
    };
    pipeline->bindDescriptorSets(commandBuffer, 0, vkUseArray(globalDescriptors), 0, nullptr);

    for (auto entity : renderableEntities) {
        auto &renderData = entity->get<MeshRenderer>();
        auto &plannerData = entity->get<PlannerData>();
        auto mesh = renderData.getMesh();

        if (!mesh) {
            continue;
        }

        if (mesh != lastMesh) {
            mesh->bind(commandBuffer);
            lastMesh = mesh;
        }

        uint32_t dyanmicOffset = plannerData.render.uniformOffset;

        std::array<vk::DescriptorSet, 1> boundDescriptors = {
            plannerData.render.buffer->set
        };

        pipeline->bindDescriptorSets(commandBuffer, 1, vkUseArray(boundDescriptors), 1, &dyanmicOffset);

        auto material = renderData.getMaterial();
        if (material) {
            pipeline->bindMaterial(commandBuffer, material);
        } else {
            pipeline->bindMaterial(commandBuffer, defaultMaterial);
        }

        commandBuffer.drawIndexed(mesh->getIndexCount(), 1, 0, 0, 0);
    }

}

void RenderPlanner::prepareFrame(uint32_t activeImage) {
    Subsystem::prepareFrame(activeImage);
}

void RenderPlanner::updateEntityUniform(Entity *entity) {
    auto &data = entity->get<PlannerData>();
    assert(data.render.buffer);

    auto &buffer = data.render.buffer;
    buffer->buffer->copyIn(
        &data.absoluteTransform,
        data.render.uniformOffset + offsetof(EntityUBO, transform),
        sizeof(glm::mat4)
    );
}

glm::mat4 RenderPlanner::getRelativeTransform(const glm::mat4 &parent, const glm::mat4 &child) {
    return parent * child;
}

void RenderPlanner::updateTransforms(Entity *entity, bool includeSelf) {
    auto &rootData = entity->get<PlannerData>();
    if (includeSelf && !entity->getParent()) {
        rootData.absoluteTransform = entity->getTransform().getTransform();
    }

    struct Data {
        PlannerData &parentData;
        Entity *entity;
    };

    std::list<Data> toVisit;

    if (includeSelf && entity->getParent()) {
        toVisit.emplace_back(entity->getParent()->get<PlannerData>(), entity);
    } else {
        for (auto &child : entity->getChildren()) {
            toVisit.emplace_back(rootData, child.get());
        }
    }

    while (!toVisit.empty()) {
        auto data = toVisit.front();
        toVisit.pop_front();
        auto &plannerData = data.entity->get<PlannerData>();

        // FIXME: This breaks the components...
        plannerData.absoluteTransform = getRelativeTransform(
            data.parentData.absoluteTransform,
            data.entity->getTransform().getTransform()
        );

        for (auto &child : data.entity->getChildren()) {
            toVisit.emplace_back(plannerData, child.get());
        }
    }
}

void RenderPlanner::prepareEntity(Entity *entity) {
    if (!entity->has<PlannerData>()) {
        ignoreComponentUpdates = true;
        entity->add<PlannerData>();
        ignoreComponentUpdates = false;
    }
}

void RenderPlanner::addLight(Entity *entity) {
    lightEntities.insert(entity);
    auto &data = entity->get<PlannerData>();

    auto pair = allocateLightUniform();
    data.light.buffer = pair.first;
    data.light.uniformOffset = pair.second;

    updateEntity(entity, EntityUpdateType::Light);
}

void RenderPlanner::removeLight(Entity *entity) {
    lightEntities.erase(entity);

    auto &data = entity->get<PlannerData>();
    if (data.light.buffer) {
        data.light.buffer->buffer->freeSection(data.light.uniformOffset, uboBufferAlignment);
        data.light.buffer = nullptr;
    }
}

LightBuffer &RenderPlanner::newLightBuffer() {
    auto uboBuffer = engine->getBufferManager().aquireDivisible(
        uboBufferMaxSize,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryUsage::eCPUToGPU
    );

    auto descriptorSets = device.allocateDescriptorSets(
        {
            objectDSPool,
            1,
            &lightDSL
        }
    );

    auto objectDS = descriptorSets[0];

    // Assign buffer to DS
    vk::DescriptorBufferInfo bufferInfo(
        uboBuffer->buffer(),
        0,
        sizeof(LightUBO)
    );

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {{
        {
            objectDS,
            Internal::StandardBindings::LightUniform,
            0, // Array element
            1, // Count
            vk::DescriptorType::eUniformBufferDynamic,
            nullptr,
            &bufferInfo
        }
    }};

    device.updateDescriptorSets(descriptorWrites, {});

    auto &buffer = lightBuffers.emplace_back();
    buffer.id = lightBuffers.size() - 1;
    buffer.buffer = std::move(uboBuffer);
    buffer.set = objectDS;

    return buffer;
}

std::pair<LightBuffer *, uint32_t> RenderPlanner::allocateLightUniform() {
    // Find the next available slot

    // TODO: Need a way to have the allocator consider the alignment requirements
    for (auto &buffer : lightBuffers) {
        auto offset = buffer.buffer->allocateSection(uboBufferAlignment);
        if (offset != ALLOCATION_FAILED) {
            return { &buffer, offset };
        }
    }

    auto &buffer = newLightBuffer();
    auto offset = buffer.buffer->allocateSection(uboBufferAlignment);
    // Sanity check
    if (offset == ALLOCATION_FAILED) {
        throw std::runtime_error("Out of object slots");
    }

    return { &buffer, offset };
}

void RenderPlanner::updateLightUniform(Entity *entity) {
    auto &data = entity->get<PlannerData>();
    auto &light = entity->get<Light>();
    assert(data.light.buffer);

    auto &buffer = data.light.buffer;
    LightUBO uniform {};
    uniform.position = entity->getTransform().getPosition();
    // TODO: Direction
    uniform.color = light.getColor();
    uniform.intensity = light.getIntensity();
    uniform.range = light.getRange();
    uniform.type = static_cast<uint32_t>(light.getType());

    buffer->buffer->copyIn(
        &uniform,
        data.render.uniformOffset,
        sizeof(LightUBO)
    );
}

void RenderPlanner::init(DeferredPipeline &pipeline) {
    deferredPipeline = &pipeline;
}

}
