#include "tech-core/pipeline.hpp"
#include "vulkanutils.hpp"
#include "tech-core/mesh.hpp"

#include <array>
#include <exception>

namespace Engine {

PipelineBuilder::PipelineBuilder(
    vk::Device device,
    vk::RenderPass renderPass,
    vk::Extent2D windowSize
) : device(device),
    renderPass(renderPass),
    windowSize(windowSize),
    depthTestEnable(true),
    depthWriteEnable(true),
    cullFaces(true),
    alpha(false)
{}

PipelineBuilder &PipelineBuilder::withVertexShader(const std::string &path) {
    vertexShaderPath = path;
    return *this;
}

PipelineBuilder &PipelineBuilder::withFragmentShader(const std::string &path) {
    fragmentShaderPath = path;
    return *this;
}

PipelineBuilder &PipelineBuilder::withGeometryType(PipelineGeometryType type) {
    geomType = type;
    return *this;
}

PipelineBuilder &PipelineBuilder::withDescriptorSet(vk::DescriptorSetLayout ds) {
    descriptorSets.push_back(ds);

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexBindingDescription(const vk::VertexInputBindingDescription &binding) {
    vertexBindings.push_back(binding);

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexBindingDescriptions(const vk::ArrayProxy<const vk::VertexInputBindingDescription> &bindings) {
    for (auto binding : bindings) {
        vertexBindings.push_back(binding);
    }

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexAttributeDescription(const vk::VertexInputAttributeDescription &attribute) {
    vertexAttributes.push_back(attribute);

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexAttributeDescriptions(const vk::ArrayProxy<const vk::VertexInputAttributeDescription> &attributes) {
    for (auto attribute : attributes) {
        vertexAttributes.push_back(attribute);
    }

    return *this;
}

PipelineBuilder &PipelineBuilder::withoutFaceCulling() {
    cullFaces = false;
    return *this;
}

PipelineBuilder &PipelineBuilder::withoutDepthWrite() {
    depthWriteEnable = false;

    return *this;
}

PipelineBuilder &PipelineBuilder::withoutDepthTest() {
    depthTestEnable = false;

    return *this;
}

PipelineBuilder &PipelineBuilder::withAlpha() {
    alpha = true;

    return *this;
}

std::unique_ptr<Pipeline> PipelineBuilder::build() {
    // Check for valid settings
    if (vertexShaderPath.empty()) {
        throw std::runtime_error("Missing vertex shader");
    }
    if (fragmentShaderPath.empty()) {
        throw std::runtime_error("Missing fragment shader");
    }

    // Set up the shader stages
    auto vertShaderCode = readFile(vertexShaderPath);
    auto fragShaderCode = readFile(fragmentShaderPath);

    vk::ShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
    vk::ShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"
    );
    
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main"
    );

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex format

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
        {},
        vkUseArray(vertexBindings),
        vkUseArray(vertexAttributes)
    );
    
    // Index format
    vk::PrimitiveTopology topology;
    switch (geomType) {
        case PipelineGeometryType::Polygons:
            topology = vk::PrimitiveTopology::eTriangleList;
            break;
        case PipelineGeometryType::SegmentedLines:
            topology = vk::PrimitiveTopology::eLineList;
            break;
        case PipelineGeometryType::ContinousLines:
            topology = vk::PrimitiveTopology::eLineStrip;
            break;
    }

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
        {},
        topology,
        VK_FALSE
    );

    // Viewport and scissor
    vk::Viewport viewport(
        0.0f,
        0.0f,
        (float)windowSize.width,
        (float)windowSize.height,
        0.0f,
        1.0f
    );

    vk::Rect2D scissor({0, 0}, windowSize);

    vk::PipelineViewportStateCreateInfo viewportState(
        {},
        1, &viewport,
        1, &scissor
    );

    // Culling
    vk::CullModeFlags cullMode;
    if (cullFaces) {
        cullMode = vk::CullModeFlagBits::eBack;
    } else {
        cullMode = vk::CullModeFlagBits::eNone;
    }
    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        cullMode,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0,
        0,
        0,
        1.0f
    );

    // Other non-configurable
    vk::PipelineMultisampleStateCreateInfo multisampling(
        {},
        vk::SampleCountFlagBits::e1,
        VK_FALSE,
        1.0f
    );

    // Blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR | 
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA
    );

    if (alpha) {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    }
    
    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {},
        VK_FALSE,
        vk::LogicOp::eCopy,
        1, &colorBlendAttachment
    );
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},
        vkUseArray(descriptorSets),
        vkUseArray(pushConstants)
    );
    
    auto pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    
    depthStencil = vk::PipelineDepthStencilStateCreateInfo(
        {},
        depthTestEnable,
        depthWriteEnable,
        vk::CompareOp::eLess,
        VK_FALSE,
        VK_FALSE,
        {},
        {},
        0.0f,
        1.0f
    );
    
    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        vkUseArray(shaderStages),
        &vertexInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        &depthStencil,
        &colorBlending,
        nullptr,
        pipelineLayout,
        renderPass,
        0,
        vk::Pipeline(),
        -1
    );

    auto pipeline = device.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);

    device.destroyShaderModule(vertShaderModule);
    device.destroyShaderModule(fragShaderModule);

    return std::unique_ptr<Pipeline>(new Pipeline(device, pipeline, pipelineLayout));
}


Pipeline::Pipeline(vk::Device device, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout)
    : device(device), pipeline(pipeline), layout(pipelineLayout)
{}

Pipeline::~Pipeline() {
    device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(layout);
}

void Pipeline::bind(vk::CommandBuffer commandBuffer) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
}

void Pipeline::bindDescriptorSets(
    vk::CommandBuffer commandBuffer, uint32_t firstSet,
    uint32_t descriptorSetCount, const vk::DescriptorSet *descriptorSets,
    uint32_t dynamicOffsetCount, const uint32_t *dynamicOffsets
) {
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        layout,
        firstSet,
        descriptorSetCount, descriptorSets, 
        dynamicOffsetCount, dynamicOffsets
    );
}

}