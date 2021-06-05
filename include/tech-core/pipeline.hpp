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
    std::shared_ptr<Image> image;
    vk::ImageLayout targetLayout { vk::ImageLayout::eShaderReadOnlyOptimal };
    std::shared_ptr<Buffer> buffer;
    bool isSamplerImmutable { false };
};

class PipelineBuilder {
    friend class RenderEngine;

public:
    PipelineBuilder &withVertexShader(const std::string &path);
    PipelineBuilder &withFragmentShader(const std::string &path);
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

    std::unique_ptr<Pipeline> build();

private:
    PipelineBuilder(
        RenderEngine &, vk::Device, vk::RenderPass, vk::Extent2D windowSize, uint32_t swapChainImages
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

    // Shader bindings
    std::vector<PipelineBinding> bindings;

    // Non-configurable
    RenderEngine &engine;
    vk::Device device;
    vk::RenderPass renderPass;
    vk::Extent2D windowSize;
    uint32_t swapChainImages;

    void processBindings(
        std::vector<vk::DescriptorSetLayout> &layouts, std::vector<uint32_t> &setCounts,
        std::vector<vk::DescriptorPoolSize> &poolSizes, uint32_t &totalSets
    );
};

class Pipeline {
    friend std::unique_ptr<Pipeline> PipelineBuilder::build();
    struct PipelineResources {
        vk::Pipeline pipeline;
        vk::PipelineLayout layout;
        std::vector<std::vector<vk::DescriptorSet>> descriptorSets;
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
    Pipeline(vk::Device, PipelineResources resources, std::map<uint32_t, PipelineBindingDetails> bindings);

    // Shared resources
    vk::Device device;

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