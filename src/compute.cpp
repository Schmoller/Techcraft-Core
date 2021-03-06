#include "tech-core/compute.hpp"
#include "tech-core/image.hpp"
#include "tech-core/buffer.hpp"

#include <utility>
#include "vulkanutils.hpp"
#include "execution_controller.hpp"

namespace Engine {

ComputeTask::ComputeTask(
    vk::Device device, const Pipeline &pipeline,
    ExecutionController &controller,
    std::map<uint32_t, ComputeTask::BindingDefinition> bindings, uint32_t xSize, uint32_t ySize, uint32_t zSize,
    size_t pushSize
) : device(device), pipeline(pipeline), controller(controller), bindings(std::move(bindings)), xSize(xSize),
    ySize(ySize), zSize(zSize), pushSize(pushSize) {

    if (pushSize > 0) {
        pushStorage = new char[pushSize];
    }
}

ComputeTask::~ComputeTask() {
    device.destroyPipeline(pipeline.pipeline);

    device.destroyShaderModule(pipeline.shader);
    device.destroyPipelineLayout(pipeline.layout);
    device.destroyDescriptorPool(pipeline.descriptorPool);
    device.destroyDescriptorSetLayout(pipeline.descriptorLayout);

    delete[] pushStorage;
}

void ComputeTask::execute(uint32_t xElements, uint32_t yElements, uint32_t zElements) {
    beginExecute();
    internalExecute(xElements, yElements, zElements);
}

void ComputeTask::push(const void *data, size_t size) {
    assert(size == pushSize);

    isUsingPushData = true;
    std::memcpy(pushStorage, data, size);
}

void ComputeTask::beginExecute() {
    isQueuedForExecution = true;
    isUsingPushData = false;

    if (!descriptorUpdates.empty()) {
        device.updateDescriptorSets(descriptorUpdates, {});

        for (auto &update : descriptorUpdates) {
            delete update.pImageInfo;
            delete update.pBufferInfo;
        }

        descriptorUpdates.clear();
    }
}

void ComputeTask::internalExecute(uint32_t xElements, uint32_t yElements, uint32_t zElements) {
    xGroupSize = xElements / xSize;
    yGroupSize = yElements / ySize;
    zGroupSize = zElements / zSize;

    controller.queueCompute(*this);
}

void ComputeTask::bindImage(uint32_t binding, const std::shared_ptr<Image> &image) {
    // TODO: Need to clear mark if removing an existing one

    boundImages[binding] = image;

    // Update the descriptor
    auto *imageInfo = new vk::DescriptorImageInfo(
        {},
        image->imageView(),
        vk::ImageLayout::eGeneral
    );

    descriptorUpdates.emplace_back(
        vk::WriteDescriptorSet(
            pipeline.descriptorSet,
            binding,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            imageInfo
        )
    );


    // auto usage = bindingUsages[binding];
    // if (usage == UsageType::Input) {
    //     image->markUsage(
    //         pipeline, vk::ShaderStageFlagBits::eCompute, vk::ImageLayout::eGeneral, vk::AccessFlagBits::eShaderRead
    //     );
    // } else {
    //     image->markUsage(
    //         pipeline, vk::ShaderStageFlagBits::eCompute, vk::ImageLayout::eGeneral, vk::AccessFlagBits::eShaderWrite
    //     );
    // }
}

void ComputeTask::bindBuffer(uint32_t binding, const std::shared_ptr<Buffer> &buffer) {
    boundBuffers[binding] = buffer;
    auto description = bindings[binding];

    auto *bufferInfo = new vk::DescriptorBufferInfo(
        buffer->buffer(),
        0,
        buffer->getSize()
    );

    vk::DescriptorType descriptorType;
    if (description.isUniform) {
        descriptorType = vk::DescriptorType::eUniformBuffer;
    } else {
        descriptorType = vk::DescriptorType::eStorageBuffer;

    }
    descriptorUpdates.emplace_back(
        vk::WriteDescriptorSet(
            pipeline.descriptorSet,
            binding,
            0,
            1,
            descriptorType,
            nullptr,
            bufferInfo
        )
    );

}

void ComputeTask::fillCommandBuffer(vk::CommandBuffer buffer) {
    buffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline.pipeline);
    buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, pipeline.layout, 0, 1, &pipeline.descriptorSet, 0, nullptr
    );

    // DEBUG: we're just going to temporarily force image layout transition. This will be through the use resource
    std::vector<vk::ImageMemoryBarrier> barriers;

    for (auto &imagePair : boundImages) {
        auto binding = bindings[imagePair.first];
        controller.useResource(
            *imagePair.second, ExecutionStage::Compute, BindPoint::Storage, static_cast<ResourceUsage>(binding.usage)
        );

        if (binding.usage == UsageType::Input) {
            imagePair.second->transition(
                buffer, vk::ImageLayout::eGeneral, true, vk::PipelineStageFlagBits::eComputeShader
            );
        } else {
            imagePair.second->transition(
                buffer, vk::ImageLayout::eGeneral, false, vk::PipelineStageFlagBits::eComputeShader
            );
        }
    }

    for (auto &bufferPair : boundBuffers) {
        auto binding = bindings[bufferPair.first];
        BindPoint bindPoint;
        if (binding.isUniform) {
            bindPoint = BindPoint::Uniform;
        } else {
            bindPoint = BindPoint::Storage;
        }
        controller.useResource(
            *bufferPair.second, ExecutionStage::Compute, bindPoint, static_cast<ResourceUsage>(binding.usage)
        );
    }

    if (isUsingPushData) {
        buffer.pushConstants(pipeline.layout, vk::ShaderStageFlagBits::eCompute, 0, pushSize, pushStorage);
    }
    buffer.dispatch(xGroupSize, yGroupSize, zGroupSize);

    for (auto &imagePair : boundImages) {
        auto binding = bindings[imagePair.first];
        if (binding.copyDest) {
            imagePair.second->transferOut(buffer, binding.copyDest.get());
        }
    }

    isQueuedForExecution = false;
}

void ComputeTask::doAfterExecution(std::function<void()> newCallback) {
    callback = std::move(newCallback);
}

void ComputeTask::notifyComplete() {
    if (callback) {
        callback();
    }

    callback = {};
}


ComputeTaskBuilder &ComputeTaskBuilder::fromFile(const char *filename, const char *symbol) {
    shaderBytes = readFile(filename);
    entryPoint = symbol;

    return *this;
}

ComputeTaskBuilder &ComputeTaskBuilder::fromBytes(const char *bytes, size_t size, const char *symbol) {
    shaderBytes.resize(size);
    std::memcpy(shaderBytes.data(), bytes, size);
    entryPoint = symbol;

    return *this;
}

ComputeTaskBuilder &ComputeTaskBuilder::fromBytes(const std::vector<char> &bytes, const char *symbol) {
    shaderBytes = bytes;
    entryPoint = symbol;

    return *this;
}

ComputeTaskBuilder &ComputeTaskBuilder::withWorkgroups(uint32_t xSize, uint32_t ySize, uint32_t zSize) {
    this->xSize = xSize;
    this->ySize = ySize;
    this->zSize = zSize;

    return *this;
}

ComputeTaskBuilder &ComputeTaskBuilder::withStorageImage(uint32_t binding, UsageType usage) {
    layoutBindings.emplace_back(
        vk::DescriptorSetLayoutBinding(
            binding,
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eCompute
        )
    );

    bindings[binding] = {
        usage, false
    };

    return *this;
}

ComputeTaskBuilder &
ComputeTaskBuilder::withStorageImage(uint32_t binding, UsageType usage, std::shared_ptr<Image> image) {
    withStorageImage(binding, usage);
    immediateImages.emplace_back(std::pair(binding, image));

    return *this;
}

ComputeTaskBuilder &ComputeTaskBuilder::withImageResultTo(uint32_t binding, std::shared_ptr<Buffer> buffer) {
    bindings[binding].copyDest = std::move(buffer);
    return *this;
}

std::vector<vk::DescriptorPoolSize>
extractPoolRequirements(const std::vector<vk::DescriptorSetLayoutBinding> &bindings) {
    std::map<vk::DescriptorType, uint32_t> counters;

    for (auto &binding : bindings) {
        auto type = binding.descriptorType;
        auto it = counters.find(type);

        if (it == counters.end()) {
            counters[type] = 1;
        } else {
            it->second++;
        }
    }

    std::vector<vk::DescriptorPoolSize> poolSizes(counters.size());
    size_t index = 0;
    for (auto &pair : counters) {
        poolSizes[index++] = {
            pair.first,
            pair.second
        };
    }

    return poolSizes;
}

std::unique_ptr<ComputeTask> ComputeTaskBuilder::build() {
    // Prepare the shader
    if (shaderBytes.empty()) {
        throw std::runtime_error("Missing shader");
    }

    vk::ShaderModule shaderModule = createShaderModule(device, shaderBytes);
    vk::PipelineShaderStageCreateInfo shaderStageInfo(
        {}, vk::ShaderStageFlagBits::eCompute, shaderModule, entryPoint.c_str()
    );

    // DescriptorSetLayout
    auto descriptorLayout = device.createDescriptorSetLayout(
        {
            {},
            vkUseArray(layoutBindings)
        }
    );

    // Prepare the pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},
        1, &descriptorLayout
    );

    size_t pushSize;
    if (pushConstant) {
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &(*pushConstant);
        pushSize = pushConstant->size;
    } else {
        pushSize = 0;
    }

    auto pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    // Create the descriptor sets
    auto poolSizes = extractPoolRequirements(layoutBindings);
    auto descriptorPool = device.createDescriptorPool(
        {
            {},
            1,
            vkUseArray(poolSizes)
        }
    );

    auto descriptorSets = device.allocateDescriptorSets(
        {
            descriptorPool,
            1, &descriptorLayout
        }
    );

    // Make the pipeline
    vk::ComputePipelineCreateInfo pipelineInfo({}, shaderStageInfo, pipelineLayout);
    auto pipeline = device.createComputePipeline(vk::PipelineCache(), pipelineInfo);

    auto task = std::unique_ptr<ComputeTask>(
        new ComputeTask(
            device,
            {
                pipeline,
                pipelineLayout,
                descriptorSets[0],
                descriptorLayout,
                descriptorPool,
                shaderModule
            },
            controller,
            bindings,
            xSize,
            ySize,
            zSize,
            pushSize
        )
    );

    // Bind any resources already provided
    for (auto &boundImage : immediateImages) {
        task->bindImage(boundImage.first, boundImage.second);
    }

    for (auto &boundBuffer : immediateStorageBuffers) {
        task->bindBuffer(boundBuffer.first, boundBuffer.second);
    }

    for (auto &boundBuffer: immediateUniformBuffers) {
        task->bindBuffer(boundBuffer.first, boundBuffer.second);
    }

    return task;
}

ComputeTaskBuilder::ComputeTaskBuilder(vk::Device device, ExecutionController &controller) : device(device),
    controller(controller) {

}


}
