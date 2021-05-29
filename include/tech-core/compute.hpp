#pragma once

#include "forward.hpp"
#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>
#include <map>
#include <optional>

namespace Engine {

enum class UsageType {
    Input,
    Output
};

class ComputeTask {
    friend class ComputeTaskBuilder;
    struct BindingDefinition {
        UsageType usage;
        bool isUniform;
    };

    struct Pipeline {
        vk::Pipeline pipeline;
        vk::PipelineLayout layout;
        vk::DescriptorSet descriptorSet;
        vk::DescriptorSetLayout descriptorLayout;
        vk::DescriptorPool descriptorPool;
        vk::ShaderModule shader;
    };

public:
    ~ComputeTask();

    void execute(uint32_t xElements, uint32_t yElements = 1, uint32_t zElements = 1);
    template<typename T>
    void execute(const T &pushData, uint32_t xElements, uint32_t yElements = 1, uint32_t zElements = 1);

    void bindImage(uint32_t binding, const std::shared_ptr<Image> &image);
    void bindBuffer(uint32_t binding, const std::shared_ptr<Buffer> &buffer);

    // This is for ExecutionController use only
    void fillCommandBuffer(vk::CommandBuffer);
private:
    // Keep track of all images we use and make sure that the images are transitioned as appropriate
    // for example, if we put in image here that we're going to write to, we need to ensure that it is in the shader write layout.

    // Provided
    vk::Device device;
    Pipeline pipeline;
    ExecutionController &controller;
    uint32_t xSize { 1 };
    uint32_t ySize { 1 };
    uint32_t zSize { 1 };
    std::map<uint32_t, BindingDefinition> bindings;
    size_t pushSize { 0 };

    // Exists for the life of the task
    char *pushStorage { nullptr };

    // queued execution
    bool isQueuedForExecution { false };
    bool isUsingPushData { false };
    uint32_t xGroupSize { 1 };
    uint32_t yGroupSize { 1 };
    uint32_t zGroupSize { 1 };

    // Bound Items
    std::map<uint32_t, std::shared_ptr<Image>> boundImages;
    std::map<uint32_t, std::shared_ptr<Buffer>> boundBuffers;
    std::vector<vk::WriteDescriptorSet> descriptorUpdates;

    ComputeTask(
        vk::Device, const Pipeline &pipeline, ExecutionController &controller,
        std::map<uint32_t, BindingDefinition> bindings, uint32_t xSize, uint32_t ySize, uint32_t zSize, size_t pushSize
    );

    void push(const void *data, size_t size);
    void beginExecute();
    void internalExecute(uint32_t xElements, uint32_t yElements, uint32_t zElements);
};

class ComputeTaskBuilder {
    friend class RenderEngine;
public:
    ComputeTaskBuilder &fromFile(const char *filename, const char *symbol = "main");
    ComputeTaskBuilder &fromBytes(const char *bytes, size_t size, const char *symbol = "main");
    ComputeTaskBuilder &fromBytes(const std::vector<char> &, const char *symbol = "main");
    template<typename T>
    ComputeTaskBuilder &withPushConstant();
    ComputeTaskBuilder &withStorageImage(uint32_t binding, UsageType usage);
    ComputeTaskBuilder &withStorageImage(uint32_t binding, UsageType usage, std::shared_ptr<Image> image);
    ComputeTaskBuilder &withStorageBuffer(uint32_t binding, UsageType usage);
    ComputeTaskBuilder &withStorageBuffer(uint32_t binding, UsageType usage, std::shared_ptr<Buffer> buffer);
    ComputeTaskBuilder &withUniformBuffer(uint32_t binding);
    ComputeTaskBuilder &withUniformBuffer(uint32_t binding, std::shared_ptr<Buffer> buffer);
    ComputeTaskBuilder &withWorkgroups(uint32_t xSize, uint32_t ySize = 1, uint32_t zSize = 1);

    std::unique_ptr<ComputeTask> build();
private:
    // Provided
    vk::Device device;
    ExecutionController &controller;

    // Configured
    std::vector<char> shaderBytes;
    std::string entryPoint;

    uint32_t xSize { 1 };
    uint32_t ySize { 1 };
    uint32_t zSize { 1 };

    // Bindings
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
    std::map<uint32_t, ComputeTask::BindingDefinition> bindings;
    std::optional<vk::PushConstantRange> pushConstant;

    // Immediately bound items
    std::vector<std::pair<uint32_t, std::shared_ptr<Image>>> immediateImages;
    std::vector<std::pair<uint32_t, std::shared_ptr<Buffer>>> immediateUniformBuffers;

    std::vector<std::pair<uint32_t, std::shared_ptr<Buffer>>> immediateStorageBuffers;

    ComputeTaskBuilder(vk::Device device, ExecutionController &controller);
};


template<typename T>
void ComputeTask::execute(const T &pushData, uint32_t xElements, uint32_t yElements, uint32_t zElements) {
    beginExecute();
    push(&pushData, sizeof(T));
    internalExecute(xElements, yElements, zElements);
}

template<typename T>
ComputeTaskBuilder &ComputeTaskBuilder::withPushConstant() {
    pushConstant = vk::PushConstantRange(
        vk::ShaderStageFlagBits::eCompute,
        0,
        sizeof(T)
    );

    return *this;
}

}



