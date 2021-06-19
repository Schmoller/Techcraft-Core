#pragma once

#include "tech-core/subsystem/base.hpp"
#include "light.hpp"
#include "tech-core/material.hpp"
#include "tech-core/mesh.hpp"
#include "tech-core/object.hpp"
#include "tech-core/pipeline.hpp"
#include "tech-core/buffer.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace Engine::Subsystem {
namespace _E = Engine;

struct ObjectCreateInfo {
    const _E::Mesh *mesh;
    const _E::Material *material;
    glm::vec3 position;
    glm::vec3 scale;
    glm::quat rotation;
};

class ObjectSubsystem;

class ObjectBuilder {
    typedef std::function<std::shared_ptr<Object>()> ObjectAllocator;

    friend class ObjectSubsystem;
public:
    ObjectBuilder &withMesh(const _E::Mesh &mesh);
    ObjectBuilder &withMaterial(const _E::Material &material);
    ObjectBuilder &withMesh(const _E::Mesh *mesh);
    ObjectBuilder &withMaterial(const _E::Material *material);
    ObjectBuilder &withPosition(const glm::vec3 &pos);
    ObjectBuilder &withScale(const glm::vec3 &scale);
    ObjectBuilder &withRotation(const glm::quat &rotation);

    ObjectBuilder &withSize(const glm::vec3 &size);
    ObjectBuilder &withTileLight(const LightCube &light);
    ObjectBuilder &withSkyTint(const LightCube &tint);
    ObjectBuilder &withOcclusion(const LightCube &occlusion);

    std::shared_ptr<Object> build();

private:
    ObjectBuilder(ObjectAllocator allocator);

    // Provided
    ObjectAllocator allocator;

    // Configuration
    glm::vec3 position { 0, 0, 0 };
    glm::quat rotation {};
    glm::vec3 scale { 1, 1, 1 };

    const _E::Mesh *mesh { nullptr };
    const _E::Material *material { nullptr };

    glm::vec3 size { 1, 1, 1 };
    LightCube tileLight;
    LightCube skyTint;
    LightCube occlusion;
};

class ObjectSubsystem : public Subsystem {
    struct ObjectBuffer {
        uint32_t id;
        std::unique_ptr<_E::DivisibleBuffer> buffer;
        vk::DescriptorSet set;
    };

public:
    static const SubsystemID<ObjectSubsystem> ID;

    // Public API
    ObjectBuilder createObject();
    std::shared_ptr<Object> getObject(uint32_t objectId);
    void removeObject(uint32_t objectId);
    void removeObject(const std::shared_ptr<Object> &object);

    bool getWireframe() const { return renderWireframe; }

    void setWireframe(bool);

    // For engine use
    void initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine);
    void initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages);
    void cleanupResources(vk::Device device, _E::RenderEngine &engine);
    void cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine);
    void writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage);
    void prepareFrame(uint32_t activeImage);

private:
    std::shared_ptr<Object> allocate();

    void initObjectBufferDSs();
    ObjectBuffer &newObjectBuffer();

    void updateObjectBuffers(bool force = false);

    _E::BufferManager *bufferManager;
    _E::MaterialManager *materialManager;
    vk::Device device;

    LightSubsystem *globalLight { nullptr };

    // Object state
    std::unordered_map<uint32_t, std::shared_ptr<Object>> objects;
    uint32_t objectIdNext = 1;
    uint32_t uboBufferAlignment;
    vk::DeviceSize uboBufferMaxSize;
    std::vector<ObjectBuffer> objectBuffers;
    std::mutex objectLock;

    // Render state
    bool renderWireframe { false };
    std::unique_ptr<_E::Pipeline> pipelineNormal;
    std::unique_ptr<_E::Pipeline> pipelineWireframe;
    vk::DescriptorSetLayout cameraAndModelDSL;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> cameraAndModelDS;

    vk::DescriptorSetLayout objectDSL;
    vk::DescriptorPool objectDSPool;
};

}