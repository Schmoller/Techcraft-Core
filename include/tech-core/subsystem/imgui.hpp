#pragma once

#include "base.hpp"

// forward declarations
class ImDrawData;

namespace Engine::Subsystem {


class ImGuiSubsystem : public Subsystem {
public:
    static const SubsystemID<ImGuiSubsystem> ID;
    static constexpr uint32_t MaxTexturesPerFrame { 40 };

    SubsystemLayer getLayer() const override { return SubsystemLayer::Overlay; }

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
    void writeBarriers(vk::CommandBuffer commandBuffer) override;
private:
    struct VertexAndIndexBuffer {
        std::unique_ptr<Buffer> vertex;
        std::unique_ptr<Buffer> index;
    };

    RenderEngine *engine;
    vk::Device device;

    std::unique_ptr<Pipeline> pipeline;
    std::unique_ptr<Pipeline> pipelineForTextures;
    vk::Sampler fontSampler;
    std::shared_ptr<Image> fontImage;
    std::vector<VertexAndIndexBuffer> vertexBuffers;

    // Current frame
    ImDrawData *drawData { nullptr };
    glm::vec2 currentScale;
    glm::vec2 currentTranslate;

    std::unordered_map<Image *, uint32_t> imagePoolMapping;
    std::unordered_map<Texture *, uint32_t> texturePoolMapping;

    void setupFont(vk::Device device);
    void
    transferVertexInformation(ImDrawData *drawData, vk::CommandBuffer commandBuffer, VertexAndIndexBuffer &buffers);
    void
    setupFrame(
        ImDrawData *drawData, Pipeline *currentPipeline, vk::CommandBuffer commandBuffer, VertexAndIndexBuffer &buffers,
        int width, int height
    );

};

}


