#pragma once

#include "forward.hpp"
#include "common_includes.hpp"
#include "pipeline.hpp"
#include <memory>

namespace Engine {

class EffectBuilder {
    friend class RenderEngine;
public:
    EffectBuilder &withShader(const std::string &path);
    template<typename T>
    EffectBuilder &withPushConstants();

    EffectBuilder &bindCamera(uint32_t set, uint32_t binding);
    EffectBuilder &bindTextures(uint32_t set, uint32_t binding);

    EffectBuilder &bindSampledImage(
        uint32_t set, uint32_t binding, vk::Sampler sampler = {}
    );
    EffectBuilder &bindSampledImage(
        uint32_t set, uint32_t binding, vk::ImageLayout imageLayout, vk::Sampler sampler = {}
    );
    EffectBuilder &bindSampledImage(
        uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::Sampler sampler = {}
    );
    EffectBuilder &bindSampledImage(
        uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::ImageLayout imageLayout,
        vk::Sampler sampler = {}
    );
    EffectBuilder &bindSampledImageImmutable(uint32_t set, uint32_t binding, vk::Sampler sampler);
    EffectBuilder &bindSampledImageImmutable(
        uint32_t set, uint32_t binding, vk::Sampler sampler, vk::ImageLayout imageLayout
    );
    EffectBuilder &bindSampledImageImmutable(
        uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::Sampler sampler
    );
    EffectBuilder &bindUniformBuffer(uint32_t set, uint32_t binding);
    EffectBuilder &bindUniformBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer);

    std::shared_ptr<Effect> build();
private:
    std::string name;
    RenderEngine &engine;
    PipelineBuilder pipelineBuilder;

    EffectBuilder(std::string name, RenderEngine &, PipelineBuilder);
};

class Effect {
    friend std::shared_ptr<Effect> EffectBuilder::build();

public:
    const std::string &getName() const { return name; }

    void bindImage(uint32_t set, uint32_t binding, const std::shared_ptr<Image> &image);
    void bindImage(uint32_t set, uint32_t binding, const std::shared_ptr<Image> &image, vk::Sampler sampler);
    void bindBuffer(uint32_t set, uint32_t binding, const std::shared_ptr<Buffer> &buffer);

    void bindCamera(uint32_t set, uint32_t binding, RenderEngine &);

    template<typename T>
    void push(
        vk::CommandBuffer commandBuffer,
        const T &constantData,
        uint32_t offset = 0
    );

    void fillFrameCommands();
    void onSwapChainRecreate(vk::RenderPass, vk::Extent2D windowSize, uint32_t subpass);
    void applyCommandBuffer(vk::CommandBuffer);

    vk::CommandBuffer &getCommandBuffer() { return buffer; }

private:
    std::string name;
    PipelineBuilder pipelineBuilder;
    std::unique_ptr<Pipeline> pipeline;
    vk::CommandBuffer buffer;

    Effect(std::string name, PipelineBuilder pipelineBuilder);
};

template<typename T>
EffectBuilder &Engine::EffectBuilder::withPushConstants() {
    return *this;
}

template<typename T>
void Effect::push(vk::CommandBuffer commandBuffer, const T &constantData, uint32_t offset) {
    pipeline->push(commandBuffer, constantData, offset);
}

}