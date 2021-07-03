#include "tech-core/post_processing.hpp"
#include "tech-core/engine.hpp"
#include "internal/packaged/effects_screen_gen_vertex_glsl.h"

#include <utility>

namespace Engine {

EffectBuilder::EffectBuilder(std::string name, RenderEngine &renderEngine, PipelineBuilder builder)
    : name(std::move(name)), engine(renderEngine), pipelineBuilder(std::move(builder)) {

    // FIXME: This is inefficient loading this every time
    pipelineBuilder
        .withVertexShader(EFFECTS_SCREEN_GEN_VERTEX_GLSL, EFFECTS_SCREEN_GEN_VERTEX_GLSL_SIZE)
        .withoutDepthTest()
        .withoutDepthWrite()
        .withoutFaceCulling()
        .withAlpha();
}

EffectBuilder &EffectBuilder::withShader(const std::string &path) {
    pipelineBuilder.withFragmentShader(path);
    return *this;
}


EffectBuilder &EffectBuilder::withShaderConstant(uint32_t constant, bool value) {
    pipelineBuilder.withShaderConstant(constant, vk::ShaderStageFlagBits::eFragment, value);

    return *this;
}

EffectBuilder &EffectBuilder::bindCamera(uint32_t set, uint32_t binding) {
    pipelineBuilder.bindCamera(set, binding);
    return *this;
}

EffectBuilder &EffectBuilder::bindTextures(uint32_t set, uint32_t binding) {
    pipelineBuilder.bindTextures(set, binding);
    return *this;
}

EffectBuilder &EffectBuilder::bindSampledImage(uint32_t set, uint32_t binding, vk::Sampler sampler) {
    pipelineBuilder.bindSampledImage(set, binding, vk::ShaderStageFlagBits::eFragment, sampler);
    return *this;
}

EffectBuilder &EffectBuilder::bindSampledImage(
    uint32_t set, uint32_t binding, vk::ImageLayout imageLayout, vk::Sampler sampler
) {
    pipelineBuilder.bindSampledImage(set, binding, vk::ShaderStageFlagBits::eFragment, imageLayout, sampler);
    return *this;
}

EffectBuilder &EffectBuilder::bindSampledImage(
    uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::Sampler sampler
) {
    pipelineBuilder.bindSampledImage(set, binding, std::move(image), vk::ShaderStageFlagBits::eFragment, sampler);
    return *this;
}

EffectBuilder &EffectBuilder::bindSampledImage(
    uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::ImageLayout imageLayout, vk::Sampler sampler
) {
    pipelineBuilder.bindSampledImage(
        set, binding, std::move(image), vk::ShaderStageFlagBits::eFragment, imageLayout, sampler
    );
    return *this;
}

EffectBuilder &EffectBuilder::bindSampledImageImmutable(uint32_t set, uint32_t binding, vk::Sampler sampler) {
    pipelineBuilder.bindSampledImageImmutable(set, binding, sampler, vk::ShaderStageFlagBits::eFragment);
    return *this;
}

EffectBuilder &EffectBuilder::bindSampledImageImmutable(
    uint32_t set, uint32_t binding, vk::Sampler sampler, vk::ImageLayout imageLayout
) {
    pipelineBuilder.bindSampledImageImmutable(set, binding, sampler, vk::ShaderStageFlagBits::eFragment, imageLayout);
    return *this;
}

EffectBuilder &EffectBuilder::bindSampledImageImmutable(
    uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::Sampler sampler
) {
    pipelineBuilder.bindSampledImageImmutable(
        set, binding, std::move(image), sampler, vk::ShaderStageFlagBits::eFragment
    );
    return *this;
}

EffectBuilder &EffectBuilder::bindUniformBuffer(uint32_t set, uint32_t binding) {
    pipelineBuilder.bindUniformBuffer(set, binding, vk::ShaderStageFlagBits::eFragment);
    return *this;
}

EffectBuilder &EffectBuilder::bindUniformBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer) {
    pipelineBuilder.bindUniformBuffer(set, binding, std::move(buffer), vk::ShaderStageFlagBits::eFragment);
    return *this;
}

std::shared_ptr<Effect> EffectBuilder::build() {
    auto effect = std::shared_ptr<Effect>(new Effect(name, pipelineBuilder));

    engine.addEffect(effect);

    return effect;
}

void Effect::bindImage(uint32_t set, uint32_t binding, const std::shared_ptr<Image> &image) {
    pipeline->bindImage(set, binding, image);
}

void Effect::bindImage(uint32_t set, uint32_t binding, const std::shared_ptr<Image> &image, vk::Sampler sampler) {
    pipeline->bindImage(set, binding, image, sampler);
}

void Effect::bindBuffer(uint32_t set, uint32_t binding, const std::shared_ptr<Buffer> &buffer) {
    pipeline->bindBuffer(set, binding, buffer);
}

void Effect::bindCamera(uint32_t set, uint32_t binding, RenderEngine &engine) {
    pipeline->bindCamera(set, binding, engine);
}

void Effect::fillFrameCommands() {
    pipeline->bind(buffer);
    vkCmdDraw(buffer, 3, 1, 0, 0);
}

void Effect::applyCommandBuffer(vk::CommandBuffer newBuffer) {
    buffer = newBuffer;
}

void Effect::onSwapChainRecreate(vk::RenderPass renderPass, vk::Extent2D windowSize, uint32_t subpass) {
    pipelineBuilder.reconfigure(renderPass, windowSize);
    pipelineBuilder.withSubpass(subpass);

    pipeline = pipelineBuilder.build();
}

Effect::Effect(std::string name, PipelineBuilder pipelineBuilder)
    : name(std::move(name)), pipelineBuilder(std::move(pipelineBuilder)) {

}

}
