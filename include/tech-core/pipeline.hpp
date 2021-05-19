#ifndef PIPELINES_HPP
#define PIPELINES_HPP

#include <unordered_map>
#include <string>

#include "common_includes.hpp"
#include <memory>

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

class Pipeline;

class PipelineBuilder {
    friend class RenderEngine;

public:
    PipelineBuilder &withVertexShader(const std::string &path);

    PipelineBuilder &withFragmentShader(const std::string &path);

    PipelineBuilder &withGeometryType(PipelineGeometryType type);

    template<typename T>
    PipelineBuilder &withPushConstants(vk::ShaderStageFlags where);

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

    std::unique_ptr<Pipeline> build();

private:
    PipelineBuilder(
        vk::Device, vk::RenderPass, vk::Extent2D windowSize
    );

    // Configurable
    PipelineGeometryType geomType = PipelineGeometryType::Polygons;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::vector<vk::PushConstantRange> pushConstants;
    std::vector<vk::DescriptorSetLayout> descriptorSets;
    std::vector<vk::DynamicState> dynamicState;
    bool depthTestEnable;
    bool depthWriteEnable;
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    bool cullFaces;
    bool alpha;
    FillMode fillMode { FillMode::Solid };

    // Non-configurable
    vk::Device device;
    vk::RenderPass renderPass;
    vk::Extent2D windowSize;
};

class Pipeline {
    friend std::unique_ptr<Pipeline> PipelineBuilder::build();

public:
    ~Pipeline();

    void bind(vk::CommandBuffer);

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
    Pipeline(vk::Device, vk::Pipeline, vk::PipelineLayout);

    // Shared resources
    vk::Device device;

    // Owned resources
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
};

template<typename T>
PipelineBuilder &PipelineBuilder::withPushConstants(vk::ShaderStageFlags where) {
    vk::PushConstantRange range(
        where,
        0,
        sizeof(T)
    );

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
        layout,
        stage,
        offset,
        sizeof(T), &constantData
    );
}

}

#endif