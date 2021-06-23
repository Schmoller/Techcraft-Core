#include <vulkanutils.hpp>
#include "render_planner.hpp"
#include "tech-core/scene/components/mesh_renderer.hpp"
#include "tech-core/buffer.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/device.hpp"
#include "components/planner_data.hpp"
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
}

void RenderPlanner::removeEntity(Entity *entity) {
    if (entity->has<MeshRenderer>()) {
        removeFromRender(entity);
    }
}

void RenderPlanner::updateEntity(Entity *entity, EntityUpdateType update) {
    if (update == EntityUpdateType::Transform) {
        updateTransforms(entity, true);
        updateEntityUniform(entity);
    } else if (update == EntityUpdateType::ComponentAdd && !ignoreComponentUpdates) {
        if (entity->has<MeshRenderer>() && !renderableEntities.contains(entity)) {
            addToRender(entity);
        }
    } else if (update == EntityUpdateType::ComponentRemove && !ignoreComponentUpdates) {
        if (!entity->has<MeshRenderer>() && renderableEntities.contains(entity)) {
            removeFromRender(entity);
        }
    }
}

void RenderPlanner::addToRender(Entity *entity) {
    renderableEntities.insert(entity);
    auto &data = entity->get<PlannerData>();

    auto pair = allocateUniform();
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
            1, // Binding
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

std::pair<EntityBuffer *, uint32_t> RenderPlanner::allocateUniform() {
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
        0, // binding
        vk::DescriptorType::eUniformBuffer,
        1, // count
        vk::ShaderStageFlagBits::eVertex
    };

    cameraAndModelDSL = device.createDescriptorSetLayout({{}, 1, &cameraBinding });

    // Holds the object information
    vk::DescriptorSetLayoutBinding objectBinding {
        1, // binding
        vk::DescriptorType::eUniformBufferDynamic,
        1, // count
        vk::ShaderStageFlagBits::eVertex
    };

    objectDSL = device.createDescriptorSetLayout({{}, 1, &objectBinding });

    size_t minimumAlignment = physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;
    uboBufferAlignment = sizeof(EntityUBO);

    uboBufferAlignment = (uboBufferAlignment + minimumAlignment - 1) & ~(minimumAlignment - 1);
    uboBufferMaxSize = physicalDevice.getProperties().limits.maxUniformBufferRange;

    this->device = device;
    this->engine = &engine;
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
                0, // Binding
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

    // Re-allocate descriptor sets
    if (entityBuffers.empty()) {
        // Nothing to allocate
        return;
    }

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
                1, // Binding
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

void RenderPlanner::cleanupResources(vk::Device device, RenderEngine &engine) {
    renderableEntities.clear();
    entityBuffers.clear();
    device.destroyDescriptorSetLayout(cameraAndModelDSL);
    device.destroyDescriptorSetLayout(objectDSL);
}

void RenderPlanner::cleanupSwapChainResources(vk::Device device, RenderEngine &engine) {

//    pipelineNormal.reset();
//    pipelineWireframe.reset();
    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorPool(objectDSPool);
}

void RenderPlanner::writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) {

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
    // TODO: Add in the texture information
}

glm::mat4 RenderPlanner::getRelativeTransform(const glm::mat4 &parent, const glm::mat4 &child) {
    return child * parent;
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

        plannerData.absoluteTransform = getRelativeTransform(
            data.parentData.absoluteTransform,
            data.entity->getTransform().getTransform()
        );

        for (auto &child : data.entity->getChildren()) {
            toVisit.emplace_back(plannerData, child.get());
        }
    }
}

}
