#pragma once

#include "base.hpp"
#include "pipeline.hpp"

#include <vector>
#include <unordered_map>

namespace Engine::Subsystem {
namespace _E = Engine;

struct SkyUBO {
    float sunAngularDiameter;
    alignas(16) glm::vec3 sunDirection;
};

class SkySubsystem : public Subsystem {
    public:
    static const SubsystemID<SkySubsystem> ID;

    SkySubsystem();

    // Public API
    void setTimeOfDay(float timePercent);
    
    // For engine use
    void initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine);
    void initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages);
    void cleanupResources(vk::Device device, _E::RenderEngine &engine);
    void cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine);
    void writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage);
    void prepareFrame(uint32_t activeImage);

    private:
    // State
    SkyUBO sky;

    // Render state
    std::unique_ptr<_E::Pipeline> pipeline;
    vk::DescriptorSetLayout descriptorLayout;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;
    std::vector<std::unique_ptr<_E::Buffer>> skyBuffers;
};

}