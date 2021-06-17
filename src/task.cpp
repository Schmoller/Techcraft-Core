#include "tech-core/task.hpp"
#include "tech-core/device.hpp"
#include "tech-core/buffer.hpp"

namespace Engine {

TaskManager::TaskManager(VulkanDevice &device)
    : device(device) {}

std::unique_ptr<Task> TaskManager::createTask() {
    vk::CommandBufferAllocateInfo allocInfo(
        device.graphicsPool,
        vk::CommandBufferLevel::ePrimary,
        1
    );

    auto commandBuffer = std::move(device.device.allocateCommandBuffersUnique(allocInfo)[0]);

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer->begin(beginInfo);

    vk::UniqueFence submitFence = device.device.createFenceUnique({});

    return std::unique_ptr<Task>(
        new Task(
            std::move(commandBuffer),
            std::move(submitFence),
            *this
        )
    );
}

vk::Fence TaskManager::submitTask(std::unique_ptr<Task> task) {
    task->commandBuffer->end();

    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &task->commandBuffer.get());
    device.graphicsQueue.queue.submit(1, &submitInfo, *task->submitFence);

    auto fence = task->submitFence.get();
    submittedTasks.push_back(std::move(task));

    return fence;
}

void TaskManager::processActions() {
    submittedTasks.erase(
        std::remove_if(
            submittedTasks.begin(),
            submittedTasks.end(),
            [this](std::unique_ptr<Task> &task) {
                if (this->device.device.waitForFences(1, &task->submitFence.get(), VK_FALSE, 0) ==
                    vk::Result::eSuccess) {
                    // Complete
                    task->executeFinishCallbacks();
                    return true;
                }
                return false;
            }
        ),
        submittedTasks.end()
    );
}


Task::Task(
    vk::UniqueCommandBuffer commandBuffer,
    vk::UniqueFence submitFence,
    TaskManager &taskManager
) : commandBuffer(std::move(commandBuffer)),
    submitFence(std::move(submitFence)),
    taskManager(taskManager) {}

void Task::execute(const std::function<void(vk::CommandBuffer)> &func) {
    func(*commandBuffer);
}

void Task::addMemoryBarrier(
    const vk::PipelineStageFlags &fromStage,
    const vk::AccessFlags &fromAccess,
    const vk::PipelineStageFlags &toStage,
    const vk::AccessFlags &toAccess
) {
    vk::MemoryBarrier barrier(fromAccess, toAccess);

    commandBuffer->pipelineBarrier(
        fromStage, toStage,
        {},
        1, &barrier, // Memory barriers
        0, nullptr, // Buffer barriers
        0, nullptr // Image barriers
    );
}

void Task::executeWhenComplete(const std::function<void()> &callback) {
    finishCallbacks.push_back(callback);
}

void Task::executeFinishCallbacks() {
    for (const auto &callback : finishCallbacks) {
        callback();
    }
}

void Task::freeWhenDone(const std::shared_ptr<Buffer> &buffer) {
    buffersToFree.push_back(buffer);
}

void Task::freeWhenDone(std::unique_ptr<Buffer> &&buffer) {
    buffersToFree.emplace_back(std::move(buffer));
}

}