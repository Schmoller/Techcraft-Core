#include "execution_controller.hpp"
#include "tech-core/device.hpp"
#include "tech-core/compute.hpp"
#include "vulkanutils.hpp"
#include <glm/glm.hpp>

namespace Engine {

ExecutionController::ExecutionController(VulkanDevice &device, uint32_t chainSize) : device(device) {
    // Allocate primary command buffers
    vk::CommandBufferAllocateInfo graphicsAllocInfo(
        device.graphicsPool,
        vk::CommandBufferLevel::ePrimary,
        chainSize
    );
    primaryCommandBuffers.graphics = device.device.allocateCommandBuffers(graphicsAllocInfo);

    vk::CommandBufferAllocateInfo computeAllocInfo(
        device.computePool,
        vk::CommandBufferLevel::ePrimary,
        chainSize
    );
    primaryCommandBuffers.compute = device.device.allocateCommandBuffers(computeAllocInfo);

    // Signal compute semaphore
    vk::SubmitInfo submitInfo;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &device.computeFinished;
    device.computeQueue.queue.submit(1, &submitInfo, {});
    device.device.waitIdle();
}

ExecutionController::~ExecutionController() {
    device.device.waitIdle();

    device.device.freeCommandBuffers(device.graphicsPool, vkUseArray(secondaryCommandBuffers.graphics));
    device.device.freeCommandBuffers(device.computePool, vkUseArray(secondaryCommandBuffers.compute));

    device.device.freeCommandBuffers(device.graphicsPool, vkUseArray(primaryCommandBuffers.graphics));
    device.device.freeCommandBuffers(device.computePool, vkUseArray(primaryCommandBuffers.compute));
}

void ExecutionController::startRender(uint32_t imageIndex) {
    currentGraphicsBuffer = primaryCommandBuffers.graphics[imageIndex];
    currentComputeBuffer = primaryCommandBuffers.compute[imageIndex];

    currentGraphicsBuffer.begin(vk::CommandBufferBeginInfo {});
    currentComputeBuffer.begin(vk::CommandBufferBeginInfo {});
    activeIndex = imageIndex;

    fillComputeBuffers();
}

void ExecutionController::beginRenderPass(
    vk::RenderPass pass, vk::Framebuffer framebuffer, vk::Extent2D screenExtent, const glm::vec4 &clear
) {
    // Prepare the command buffers
//    vk::CommandBufferInheritanceInfo cbInheritance(pass, 0, framebuffer);

    // Begin the render pass itself
    std::array<vk::ClearValue, 2> clearValues {
        vk::ClearColorValue(std::array<float, 4> { clear.r, clear.g, clear.b, clear.a }),
        vk::ClearDepthStencilValue(1.0f, 0.0f)
    };

    vk::RenderPassBeginInfo renderPassInfo(
        pass,
        framebuffer,
        {{ 0, 0 }, screenExtent },
        vkUseArray(clearValues)
    );

    currentGraphicsBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);
}

void ExecutionController::addToRender(vk::CommandBuffer buffer) {
    currentGraphicsBuffer.executeCommands(1, &buffer);
}

void ExecutionController::endRenderPass() {
    currentGraphicsBuffer.endRenderPass();
}

//void ExecutionController::prepareCommands(uint32_t activeIndex) {
//    auto graphicsBufferPrimary = primaryCommandBuffers.graphics[activeIndex];
//    auto computeBufferPrimary = primaryCommandBuffers.compute[activeIndex];
//
//
//    /*
//     * Here is where we should do all of the filling of the command buffers
//     * After this entire process is done then we should look at what resources have been bound
//     * Where do we handle the render pass stuff?
//     */
//    // TODO: Do the memory barriers for all the resources needed here
//    // TODO: Do any memory barriers for transfer release here
//}

void ExecutionController::endRender() {
    currentGraphicsBuffer.end();
    currentComputeBuffer.end();

    // TODO: Automatically work out the wait stages based on where bound resources depend on eachother
    std::array<vk::PipelineStageFlags, 2> graphicsWaitStages {
        vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    std::array<vk::Semaphore, 2> graphicsWait { device.computeFinished, device.presentFinished };
    std::array<vk::Semaphore, 2> graphicsSignal { device.renderFinished, device.readyForCompute };

    vk::SubmitInfo graphicsSubmitInfo(
        vkUseArray(graphicsWait),
        graphicsWaitStages.data(),
        1, &currentGraphicsBuffer,
        vkUseArray(graphicsSignal)
    );

    device.device.resetFences(1, &device.renderReady);
    device.graphicsQueue.queue.submit(1, &graphicsSubmitInfo, device.renderReady);

    // fire off the compute stage
    std::array<vk::Semaphore, 1> computeWait { device.readyForCompute };
    vk::PipelineStageFlags computeWaitStage = vk::PipelineStageFlagBits::eComputeShader;
    vk::SubmitInfo computeSubmitInfo(
        vkUseArray(computeWait),
        &computeWaitStage,
        1, &currentComputeBuffer,
        1, &device.computeFinished
    );

    device.device.resetFences(1, &device.computeReady);
    device.computeQueue.queue.submit(1, &computeSubmitInfo, device.computeReady);

    for (auto task : queuedComputeTasks) {
        task->notifyComplete();
    }
    queuedComputeTasks.clear();
}

void
ExecutionController::useResource(const Image &image, ExecutionStage where, BindPoint bindPoint, ResourceUsage usage) {

}

void
ExecutionController::useResource(const Buffer &buffer, ExecutionStage where, BindPoint bindPoint, ResourceUsage usage) {

}

void ExecutionController::queueCompute(ComputeTask &task) {
    queuedComputeTasks.push_back(&task);
}

vk::CommandBuffer ExecutionController::acquireSecondaryGraphicsCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo(
        device.graphicsPool,
        vk::CommandBufferLevel::eSecondary,
        1
    );
    auto buffers = device.device.allocateCommandBuffers(allocInfo);
    secondaryCommandBuffers.graphics.push_back(buffers[0]);
    return buffers[0];
}

vk::CommandBuffer ExecutionController::acquireSecondaryComputeCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo(
        device.computePool,
        vk::CommandBufferLevel::eSecondary,
        1
    );
    auto buffers = device.device.allocateCommandBuffers(allocInfo);
    secondaryCommandBuffers.compute.push_back(buffers[0]);
    return buffers[0];
}

void ExecutionController::fillComputeBuffers() {
    for (auto task : queuedComputeTasks) {
        task->fillCommandBuffer(currentComputeBuffer);
    }
}

void ExecutionController::addBarriers(Subsystem::Subsystem &subsystem) {
    subsystem.writeBarriers(currentGraphicsBuffer);
}


}
