#pragma once

#include "base.hpp"

namespace Engine::Subsystem {

struct LightUBO {
    glm::vec3 globalSkyLight;
    alignas(16) glm::vec3 ambientLight;
};

class LightSubsystem : public Subsystem {
    public:
    static const SubsystemID<LightSubsystem> ID;

    LightSubsystem();

    // Public API
    void setTimeOfDay(float timePercent);

    vk::DescriptorSetLayout layout() const;
    vk::DescriptorSet descriptor(uint32_t activeImage) const;

    // For engine use
    void initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine);
    void initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages);
    void cleanupResources(vk::Device device, _E::RenderEngine &engine);
    void cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine);
    void writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage);
    void prepareFrame(uint32_t activeImage);

    private:
    // Provided
    _E::BufferManager *bufferManager;
    
    // State
    LightUBO light;

    // Render state
    vk::DescriptorSetLayout descriptorLayout;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;
    std::vector<std::unique_ptr<_E::Buffer>> lightBuffers;
};


}