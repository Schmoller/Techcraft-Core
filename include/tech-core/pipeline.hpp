#ifndef PIPELINES_HPP
#define PIPELINES_HPP

#include "forward.hpp"
#include <unordered_map>

#include <string>
#include "common_includes.hpp"
#include <memory>
#include <map>

namespace Engine {

enum class PipelineGeometryType {
    /**
     * Standard geometry.
     */
    Polygons,
    /**
     * Individual line segments
     */
    SegmentedLines,
    /**
     * A continuous line involving multiple connected segments
     */
    ContinousLines
};

enum class FillMode {
    Solid,
    Wireframe,
    Point
};

enum class BindingCount {
    Single,
    PerSwapChain,
    Pool,
    External
};

enum class SpecialBinding {
    None,
    Camera,
    Textures
};

struct PipelineBinding {
    uint32_t set { 0 };
    uint32_t binding { 0 };
    BindingCount count { BindingCount::Single };
    vk::DescriptorSetLayoutBinding definition;
    SpecialBinding type { SpecialBinding::None };

    vk::Sampler sampler;
    std::weak_ptr<Image> image;
    vk::ImageLayout targetLayout { vk::ImageLayout::eShaderReadOnlyOptimal };
    std::weak_ptr<Buffer> buffer;
    bool isSamplerImmutable { false };
    uint32_t poolSize { 0 };
};

class PipelineBuilder {
    friend class RenderEngine;
    friend class Effect;

public:
    PipelineBuilder &withVertexShader(const std::string &path);
    PipelineBuilder &withFragmentShader(const std::string &path);
    PipelineBuilder &withShaderConstant(uint32_t constantId, vk::ShaderStageFlagBits stage, bool);
    template<typename T>
    PipelineBuilder &withShaderConstant(uint32_t constantId, vk::ShaderStageFlagBits stage, T);

    PipelineBuilder &withGeometryType(PipelineGeometryType type);
    template<typename T>
    PipelineBuilder &withPushConstants(const vk::ShaderStageFlags &where);

    PipelineBuilder &withDescriptorSet(vk::DescriptorSetLayout ds);
    PipelineBuilder &withoutDepthWrite();
    PipelineBuilder &withoutDepthTest();
    PipelineBuilder &withVertexBindingDescription(const vk::VertexInputBindingDescription &);
    PipelineBuilder &withVertexBindingDescriptions(const vk::ArrayProxy<const vk::VertexInputBindingDescription> &);
    PipelineBuilder &withVertexAttributeDescription(const vk::VertexInputAttributeDescription &);
    PipelineBuilder &withVertexAttributeDescriptions(const vk::ArrayProxy<const vk::VertexInputAttributeDescription> &);
    PipelineBuilder &withoutFaceCulling();
    PipelineBuilder &withAlpha();
    PipelineBuilder &withFillMode(FillMode);
    PipelineBuilder &withDynamicState(vk::DynamicState);
    PipelineBuilder &withSubpass(uint32_t);

    PipelineBuilder &bindCamera(uint32_t set, uint32_t binding);
    PipelineBuilder &bindTextures(uint32_t set, uint32_t binding);
    PipelineBuilder &bindSampledImage(
        uint32_t set, uint32_t binding,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eFragment, vk::Sampler sampler = {}
    );
    PipelineBuilder &bindSampledImage(
        uint32_t set, uint32_t binding,
        const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout, vk::Sampler sampler = {}
    );
    PipelineBuilder &bindSampledImage(
        uint32_t set, uint32_t binding, std::shared_ptr<Image> image,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eFragment,
        vk::Sampler sampler = {}
    );
    PipelineBuilder &bindSampledImage(
        uint32_t set, uint32_t binding, std::shared_ptr<Image> image,
        const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout,
        vk::Sampler sampler = {}
    );
    PipelineBuilder &bindSampledImageImmutable(
        uint32_t set, uint32_t binding, vk::Sampler sampler,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eFragment
    );
    PipelineBuilder &bindSampledImageImmutable(
        uint32_t set, uint32_t binding, vk::Sampler sampler,
        const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout
    );
    PipelineBuilder &bindSampledImageImmutable(
        uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::Sampler sampler,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eFragment
    );
    PipelineBuilder &bindSampledImageImmutable(
        uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::Sampler sampler,
        const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout
    );
    PipelineBuilder &bindUniformBuffer(
        uint32_t set, uint32_t binding, const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eVertex
    );
    PipelineBuilder &bindUniformBuffer(
        uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eVertex
    );
    PipelineBuilder &bindUniformBufferDynamic(
        uint32_t set, uint32_t binding, const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eVertex
    );
    PipelineBuilder &bindUniformBufferDynamic(
        uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eVertex
    );
    PipelineBuilder &bindSampledImagePool(
        uint32_t set, uint32_t binding, uint32_t size,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eFragment,
        vk::Sampler sampler = {}
    );
    PipelineBuilder &bindSampledImagePool(
        uint32_t set, uint32_t binding, uint32_t size, const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout,
        vk::Sampler sampler = {}
    );
    PipelineBuilder &bindSampledImagePoolImmutable(
        uint32_t set, uint32_t binding, uint32_t size, vk::Sampler sampler,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eFragment
    );
    PipelineBuilder &bindSampledImagePoolImmutable(
        uint32_t set, uint32_t binding, uint32_t size, vk::Sampler sampler, const vk::ShaderStageFlags &stages,
        vk::ImageLayout imageLayout
    );
    PipelineBuilder &withInputAttachment(
        uint32_t set, uint32_t binding, const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eFragment
    );
    PipelineBuilder &withInputAttachment(
        uint32_t set, uint32_t binding, std::shared_ptr<Image> image,
        const vk::ShaderStageFlags &stages = vk::ShaderStageFlagBits::eFragment
    );

    std::unique_ptr<Pipeline> build();

private:
    PipelineBuilder(
        RenderEngine &, vk::Device, vk::RenderPass, vk::Extent2D windowSize, uint32_t swapChainImages,
        Internal::DescriptorCacheManager &
    );

    // Configurable
    PipelineGeometryType geomType { PipelineGeometryType::Polygons };
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::vector<vk::PushConstantRange> pushConstants;
    std::vector<vk::DescriptorSetLayout> providedDescriptorLayouts;
    std::vector<vk::DynamicState> dynamicState;
    bool depthTestEnable;
    bool depthWriteEnable;
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    bool cullFaces;
    bool alpha;
    FillMode fillMode { FillMode::Solid };
    size_t pushOffset { 0 };
    uint32_t subpass { 0 };

    // Shader bindings
    std::vector<PipelineBinding> bindings;

    // FIXME: We should break shaders out into own class
    std::vector<uint32_t> fragmentSpecializationData;
    std::vector<vk::SpecializationMapEntry> fragmentSpecializationEntries;
    std::vector<uint32_t> vertexSpecializationData;
    std::vector<vk::SpecializationMapEntry> vertexSpecializationEntries;

    // Non-configurable
    RenderEngine &engine;
    vk::Device device;
    vk::RenderPass renderPass;
    vk::Extent2D windowSize;
    uint32_t swapChainImages;
    Internal::DescriptorCacheManager &descriptorManager;

    void processBindings(
        std::vector<vk::DescriptorSetLayout> &layouts, std::vector<uint32_t> &setCounts,
        std::vector<vk::DescriptorPoolSize> &poolSizes, uint32_t &totalSets, std::vector<bool> &autoBindSet
    );

    void reconfigure(vk::RenderPass, vk::Extent2D windowSize);
};

class Pipeline {
    friend std::unique_ptr<Pipeline> PipelineBuilder::build();
    struct PipelineResources {
        vk::Pipeline pipeline;
        vk::PipelineLayout layout;
        std::vector<std::vector<vk::DescriptorSet>> descriptorSets;
        std::vector<bool> autoBindSet;
        std::vector<vk::DescriptorSetLayout> descriptorLayouts;
        vk::DescriptorPool descriptorPool;
    };

    struct PipelineBindingDetails {
        vk::DescriptorType type;
        vk::ImageLayout targetLayout;
        vk::Sampler sampler;
    };

public:
    ~Pipeline();

    void bindImage(uint32_t set, uint32_t binding, const std::shared_ptr<Image> &image);
    void bindImage(uint32_t set, uint32_t binding, const std::shared_ptr<Image> &image, vk::Sampler sampler);
    void bindBuffer(uint32_t set, uint32_t binding, const std::shared_ptr<Buffer> &buffer);

    void bindCamera(uint32_t set, uint32_t binding, RenderEngine &);

    void bind(vk::CommandBuffer, uint32_t activeImage = 0);

    void bindTexture(vk::CommandBuffer, uint32_t set, const Texture *);

    void bindPoolImage(vk::CommandBuffer commandBuffer, uint32_t set, uint32_t binding, uint32_t index);
    void updatePoolImage(uint32_t set, uint32_t binding, uint32_t index, const Image &image);
    void updatePoolImage(uint32_t set, uint32_t binding, uint32_t index, const Image &image, vk::ImageLayout layout);
    void updatePoolImage(uint32_t set, uint32_t binding, uint32_t index, vk::ImageView image);
    void updatePoolImage(uint32_t set, uint32_t binding, uint32_t index, vk::ImageView image, vk::ImageLayout layout);

    void bindDescriptorSets(
        vk::CommandBuffer commandBuffer, uint32_t firstSet,
        uint32_t descriptorSetCount, const vk::DescriptorSet *descriptorSets,
        uint32_t dynamicOffsetCount, const uint32_t *dynamicOffsets
    );

    template<typename T>
    void push(
        vk::CommandBuffer commandBuffer,
        vk::ShaderStageFlags stage,
        const T &constantData,
        uint32_t offset = 0
    );

private:
    Pipeline(
        vk::Device, PipelineResources resources, std::map<uint32_t, PipelineBindingDetails> bindings,
        std::shared_ptr<Internal::DescriptorCache> descriptorCache
    );


    // Shared resources
    vk::Device device;
    std::shared_ptr<Internal::DescriptorCache> descriptorCache;

    // Owned resources
    PipelineResources resources;
    std::map<uint32_t, PipelineBindingDetails> bindings;

    // Bound resources
    std::map<uint32_t, std::shared_ptr<Image>> boundImages;
    std::map<uint32_t, std::shared_ptr<Buffer>> boundBuffers;
    std::vector<vk::WriteDescriptorSet> descriptorUpdates;
};

template<typename T>
PipelineBuilder &PipelineBuilder::withPushConstants(const vk::ShaderStageFlags &where) {
    vk::PushConstantRange range(
        where,
        pushOffset,
        sizeof(T)
    );

    pushOffset += sizeof(T);

    pushConstants.push_back(range);

    return *this;
}


template<typename T>
PipelineBuilder &PipelineBuilder::withShaderConstant(uint32_t constantId, vk::ShaderStageFlagBits stage, T value) {
    static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Can only use types that are a multiple of 4 bytes");

    constexpr uint32_t numSlots = sizeof(T) / sizeof(uint32_t);

    std::vector<uint32_t> *specializationData;
    std::vector<vk::SpecializationMapEntry> *specializationEntries;

    if (stage == vk::ShaderStageFlagBits::eVertex) {
        specializationData = &vertexSpecializationData;
        specializationEntries = &vertexSpecializationEntries;
    } else if (stage == vk::ShaderStageFlagBits::eFragment) {
        specializationData = &fragmentSpecializationData;
        specializationEntries = &fragmentSpecializationEntries;
    } else {
        throw std::runtime_error("Invalid stage");
    }

    uint32_t offset = sizeof(uint32_t) * specializationData->size();
    auto *interpreted = reinterpret_cast<uint32_t *>(&value);

    if constexpr (numSlots == 1) {
        specializationData->push_back(*interpreted);
    } else {
        for (uint32_t slot = 0; slot < numSlots; ++slot) {
            specializationData->push_back(interpreted[slot]);
        }
    }
    specializationEntries->emplace_back(constantId, offset, sizeof(T));

    return *this;
}

template<typename T>
void Pipeline::push(
    vk::CommandBuffer commandBuffer,
    vk::ShaderStageFlags stage,
    const T &constantData,
    uint32_t offset
) {
    commandBuffer.pushConstants(
        resources.layout,
        stage,
        offset,
        sizeof(T), &constantData
    );
}

}

#endif