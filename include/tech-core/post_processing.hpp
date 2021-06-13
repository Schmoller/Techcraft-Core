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

//    EffectBuilder &withShaderConstantBool(uint32_t constant, bool);
//    EffectBuilder &withShaderConstantInt(uint32_t constant, int32_t);
//    EffectBuilder &withShaderConstantUint(uint32_t constant, uint32_t);
//    EffectBuilder &withShaderConstantFloat(uint32_t constant, float);
//    EffectBuilder &withShaderConstantDouble(uint32_t constant, double);

    EffectBuilder &withShaderConstant(uint32_t constant, bool);
    template<typename T>
    EffectBuilder &withShaderConstant(uint32_t constant, T);

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

    std::vector<uint32_t> specializationData;
    std::vector<vk::SpecializationMapEntry> specializationEntries;

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
    // FIXME: We didnt end up adding this...
    return *this;
}

template<typename T>
EffectBuilder &EffectBuilder::withShaderConstant(uint32_t constant, T value) {
    pipelineBuilder.withShaderConstant(constant, vk::ShaderStageFlagBits::eFragment, value);
    return *this;
}


template<typename T>
void Effect::push(vk::CommandBuffer commandBuffer, const T &constantData, uint32_t offset) {
    pipeline->push(commandBuffer, constantData, offset);
}

}