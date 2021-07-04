#pragma once

#include "tech-core/scene/entity.hpp"
#include "tech-core/subsystem/base.hpp"
#include "internal.hpp"
#include "pipelines/deferred_pipeline.hpp"
#include <vulkan/vulkan.hpp>
#include <unordered_set>

namespace Engine::Internal {

struct EntityUBO {
    alignas(16) glm::mat4 transform;
};

struct LightUBO {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float range;
    uint32_t type;
};

enum class EntityUpdateType {
    Transform,
    ComponentAdd,
    ComponentRemove,
    Light,
    Other
};

class RenderPlanner : public Subsystem::Subsystem {
public:
    static const Engine::Subsystem::SubsystemID<RenderPlanner> ID;

    Engine::Subsystem::SubsystemLayer getLayer() const override { return Engine::Subsystem::SubsystemLayer::BeforePasses; }

    void init(DeferredPipeline &deferredPipeline);
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
    DeferredPipeline *deferredPipeline { nullptr };

    bool ignoreComponentUpdates { false };

    // TODO: Eventually replace this with a QuadTree or other spacial partitioning structure
    std::unordered_set<Entity *> renderableEntities;
    std::unordered_set<Entity *> lightEntities;
    std::unique_ptr<Pipeline> pipelineNormal;

    vk::DeviceSize uboBufferAlignment;
    vk::DeviceSize uboBufferMaxSize;
    std::vector<EntityBuffer> entityBuffers;
    std::vector<LightBuffer> lightBuffers;

    vk::DescriptorSetLayout cameraAndModelDSL;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> cameraAndModelDS;

    vk::DescriptorSetLayout objectDSL;
    vk::DescriptorPool objectDSPool;

    vk::DescriptorSetLayout lightDSL;

    const Material *defaultMaterial;

    void addToRender(Entity *);
    void removeFromRender(Entity *);
    void addLight(Entity *);
    void removeLight(Entity *);
    EntityBuffer &newEntityBuffer();
    std::pair<EntityBuffer *, uint32_t> allocateEntityUniform();
    void updateEntityUniform(Entity *);
    LightBuffer &newLightBuffer();
    std::pair<LightBuffer *, uint32_t> allocateLightUniform();
    void updateLightUniform(Entity *);
    static glm::mat4 getRelativeTransform(const glm::mat4 &parent, const glm::mat4 &child);
    void updateTransforms(Entity *, bool includeSelf);
};

}



