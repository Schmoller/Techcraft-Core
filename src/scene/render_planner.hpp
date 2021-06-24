#pragma once

#include "tech-core/scene/entity.hpp"
#include "internal.hpp"
#include <vulkan/vulkan.hpp>
#include <unordered_set>
#include "tech-core/subsystem/base.hpp"

namespace Engine::Internal {

struct EntityUBO {
    alignas(16) glm::mat4 transform;
    alignas(4)  uint32_t textureIndex;
};


enum class EntityUpdateType {
    Transform,
    ComponentAdd,
    ComponentRemove,
    Other
};

class RenderPlanner : public Subsystem::Subsystem {
public:
    static const Engine::Subsystem::SubsystemID<RenderPlanner> ID;

    void initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, RenderEngine &engine) override;
    void initialiseSwapChainResources(vk::Device device, RenderEngine &engine, uint32_t swapChainImages) override;
    void cleanupResources(vk::Device device, RenderEngine &engine) override;
    void cleanupSwapChainResources(vk::Device device, RenderEngine &engine) override;
    void writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) override;
    void prepareFrame(uint32_t activeImage) override;
    void prepareEntity(Entity *);

    void addEntity(Entity *);
    void removeEntity(Entity *);
    void updateEntity(Entity *, EntityUpdateType);
private:
    RenderEngine *engine;
    vk::Device device;

    bool ignoreComponentUpdates { false };

    // TODO: Eventually replace this with a QuadTree or other spacial partitioning structure
    std::unordered_set<Entity *> renderableEntities;
    std::unique_ptr<Pipeline> pipelineNormal;

    vk::DeviceSize uboBufferAlignment;
    vk::DeviceSize uboBufferMaxSize;
    std::vector<EntityBuffer> entityBuffers;

    vk::DescriptorSetLayout cameraAndModelDSL;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> cameraAndModelDS;

    vk::DescriptorSetLayout objectDSL;
    vk::DescriptorPool objectDSPool;

    void addToRender(Entity *);
    void removeFromRender(Entity *);
    EntityBuffer &newEntityBuffer();
    std::pair<EntityBuffer *, uint32_t> allocateUniform();
    void updateEntityUniform(Entity *);
    static glm::mat4 getRelativeTransform(const glm::mat4 &parent, const glm::mat4 &child);
    void updateTransforms(Entity *, bool includeSelf);
};

}



