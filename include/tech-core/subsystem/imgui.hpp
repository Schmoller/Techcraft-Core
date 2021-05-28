#pragma once

#include "base.hpp"

// forward declarations
class ImDrawData;

namespace Engine::Subsystem {

class ImGuiSubsystem : public Subsystem {
public:
    static const SubsystemID<ImGuiSubsystem> ID;

    bool hasMouseFocus() const;
    bool hasKeyboardFocus() const;

    void initialiseWindow(GLFWwindow *window) override;
    void initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) override;
    void initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) override;
    void cleanupResources(vk::Device device, _E::RenderEngine &engine) override;
    void cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) override;
    void writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) override;
    void prepareFrame(uint32_t activeImage) override;
    void beginFrame() override;
private:
    struct VertexAndIndexBuffer {
        std::unique_ptr<Buffer> vertex;
        std::unique_ptr<Buffer> index;
    };

    RenderEngine *engine;
    std::unique_ptr<Pipeline> pipeline;
    vk::DescriptorSetLayout imageSamplerLayout;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> imageSamplerDS;
    vk::Sampler fontSampler;
    std::shared_ptr<Image> fontImage;
    std::vector<VertexAndIndexBuffer> vertexBuffers;
    ImDrawData *drawData { nullptr };

    void setupFont(vk::Device device);
    void
    transferVertexInformation(ImDrawData *drawData, vk::CommandBuffer commandBuffer, VertexAndIndexBuffer &buffers);
    void
    setupFrame(
        ImDrawData *drawData, vk::CommandBuffer commandBuffer, VertexAndIndexBuffer &buffers, int width, int height
    );

};

}


