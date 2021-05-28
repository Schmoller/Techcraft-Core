#include "execution_controller.hpp"
#include "tech-core/device.hpp"

namespace Engine {

ExecutionController::ExecutionController(VulkanDevice &device) : device(device) {
    vk::CommandBufferAllocateInfo allocateInfo(
        device.computePool,
        vk::CommandBufferLevel::ePrimary,
        1
    );

    auto commandBuffers = device.device.allocateCommandBuffers(allocateInfo);
    commandBuffer = commandBuffers[0];
}

ExecutionController::~ExecutionController() {
    device.computeQueue.queue.waitIdle();

    device.device.freeCommandBuffers(device.computePool, 1, &commandBuffer);
}

void ExecutionController::beginFrame() {

}

void ExecutionController::endFrame() {
    if (hasComputeThisFrame) {
        commandBuffer.end();
        hasComputeThisFrame = false;

        vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eComputeShader;
        vk::SubmitInfo submitInfo(
//            1, &device.renderFinished,
            0, nullptr,
            &flags,
            1, &commandBuffer,
            1, &device.computeFinished
        );

        device.device.resetFences(1, &device.computeReady);
        device.computeQueue.queue.submit(1, &submitInfo, device.computeReady);
    }
}

void
ExecutionController::useResource(const Image &image, ExecutionStage where, BindPoint bindPoint, ResourceUsage usage) {

}

void
ExecutionController::useResource(const Buffer &buffer, ExecutionStage where, BindPoint bindPoint, ResourceUsage usage) {

}

vk::CommandBuffer ExecutionController::beginExecution(const ComputeTask &task) {
    if (!hasComputeThisFrame) {
        initializeCompute();
    }

    return commandBuffer;
}

void ExecutionController::initializeCompute() {
    vk::CommandBufferBeginInfo info;
    commandBuffer.begin(&info);

    hasComputeThisFrame = true;
}

}