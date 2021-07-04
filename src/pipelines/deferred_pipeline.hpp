#pragma once

#include <vulkan/vulkan.hpp>
#include "tech-core/forward.hpp"

namespace Engine::Internal {

class DeferredPipeline {
public:
    DeferredPipeline(RenderEngine &engine, VulkanDevice &device, ExecutionController &controller);
    ~DeferredPipeline();

    void cleanupSwapChain();
    void recreateSwapChain(
        std::vector<vk::ImageView> passOutputImages, vk::Format swapChainFormat, vk::Extent2D framebufferSize,
        const std::shared_ptr<Image> &depth
    );

    void begin(uint32_t imageIndex);

    void beginGeometry();
    void renderGeometry(const Entity *);
    void endGeometry();

    void beginLighting();
    void renderLight(const Entity *);
    void endLighting();

    void end();
private:
    // External
    VulkanDevice &device;
    RenderEngine &engine;
    vk::Format swapChainFormat;
    std::vector<vk::ImageView> passOutputImages;
    ExecutionController &controller;
    vk::Format depthFormat;
    vk::Extent2D framebufferSize;

    // Cached
    const Material *defaultMaterial;

    // Owned
    vk::RenderPass renderPass;
    std::vector<vk::Framebuffer> framebuffers;
    std::shared_ptr<Image> attachmentDiffuseOcclusion;
    std::shared_ptr<Image> attachmentNormalRoughness;
    std::shared_ptr<Image> attachmentPosition;

    std::unique_ptr<Pipeline> geometryPipeline;
    std::unique_ptr<Pipeline> fullScreenLightingPipeline;
    std::unique_ptr<Pipeline> worldLightingPipeline;

    // Transient
    vk::CommandBuffer geometryCommandBuffer;
    vk::CommandBuffer lightingCommandBuffer;
    const Mesh *lastMesh { nullptr };
    uint32_t activeImage { 0 };
    vk::Framebuffer activeFramebuffer;

    std::vector<const Entity *> fullScreenLights;
    std::vector<const Entity *> worldLights;

    void createAttachments();
    void createRenderPass();
    void createFramebuffers(const Image *depthImage);
    void createLightingPipeline(const std::shared_ptr<Image> &depth);
    void createGeometryPipeline();
};

}



