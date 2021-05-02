#pragma once

#include "base.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "object.hpp"
#include "pipeline.hpp"
#include "buffer.hpp"

#include <vector>


namespace Engine::Subsystem {
namespace _E = Engine;

struct DebugLinePC {
    glm::vec4 from;
    glm::vec4 to;
    glm::vec4 color;
};

class DebugSubsystem : public Subsystem {
    public:
    static const SubsystemID<DebugSubsystem> ID;
    static DebugSubsystem *instance();
    DebugSubsystem();

    // Public API
    void debugDrawLine(const glm::vec3 &from, const glm::vec3 &to, uint32_t color = 0xFFFFFF);
    void debugDrawBox(const glm::vec3 &from, const glm::vec3 &to, uint32_t color = 0xFFFFFF);

    // For engine use
    void initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine);
    void initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages);
    void cleanupResources(vk::Device device, _E::RenderEngine &engine);
    void cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine);
    void writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage);
    void afterFrame(uint32_t activeImage);

    private:
    static DebugSubsystem *inst;

    // State
    std::vector<DebugLinePC> debugDrawCmds;

    // Render state
    std::unique_ptr<_E::Pipeline> pipeline;
    vk::DescriptorSetLayout cameraOnlyDSL;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> cameraOnlyDS;
};


}