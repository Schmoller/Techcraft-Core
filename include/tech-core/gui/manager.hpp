#pragma once

#include <vulkan/vulkan.hpp>

#include "tech-core/forward.hpp"
#include "common.hpp"

#include <memory>
#include <unordered_map>
#include <limits>
#include <vector>

namespace Engine::Gui {

constexpr vk::DeviceSize MAX_GUI_BUFFER_SIZE = 0xFFFFF;

struct ComponentMapping {
    struct Region {
        const Texture *texture { nullptr };
        // Tracking for buffer usage
        vk::DeviceSize offset;
        vk::DeviceSize size;
        vk::DeviceSize vertCount;
        vk::DeviceSize indexCount;
        vk::DeviceSize indexOffset;
    };

    uint16_t id;
    std::shared_ptr<BaseComponent> component;
    std::vector<Region> regions;
};

struct GuiPC {
    glm::mat4 view;
};

struct FreeSpace {
    vk::DeviceSize offset;
    vk::DeviceSize size;
};

class GuiManager {
public:
    GuiManager(
        vk::Device device,
        Engine::TextureManager &textureManager,
        Engine::BufferManager &bufferManager,
        Engine::TaskManager &taskManager,
        Engine::FontManager &fontManager,
        Engine::PipelineBuilder pipelineBuilder,
        const vk::Extent2D &windowSize
    );

    void recreatePipeline(Engine::PipelineBuilder pipelineBuilder, const vk::Extent2D &windowSize);

    uint16_t addComponent(std::shared_ptr<BaseComponent> component);
    void removeComponent(uint16_t id);
    void updateComponent(uint16_t id);

    void update();

    void render(vk::CommandBuffer commandBuffer, vk::CommandBufferInheritanceInfo &cbInheritance);

private:
    void renderComponent(BaseComponent *component, ComponentMapping &mapping);
    void markComponentDirty(uint16_t id);
    void processAnchors(BaseComponent *component);

    // Provided
    vk::Device device;
    Engine::TextureManager &textureManager;
    Engine::BufferManager &bufferManager;
    Engine::TaskManager &taskManager;
    Engine::FontManager &fontManager;
    vk::Extent2D windowSize;

    // resources
    std::unique_ptr<Engine::Pipeline> pipeline;
    vk::UniqueSampler sampler;
    vk::UniqueDescriptorSetLayout transformDSL;
    vk::UniqueDescriptorSet transformDS;
    vk::UniqueDescriptorPool dsPool;

    // State
    std::unordered_map<uint16_t, ComponentMapping> components;
    uint16_t nextId = 0;
    std::vector<uint16_t> dirtyComponents;

    // For rendering
    std::unique_ptr<Engine::DivisibleBuffer> combinedVertexIndexBuffer;
    GuiPC viewState;
};

}