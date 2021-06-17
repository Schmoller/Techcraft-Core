#pragma once

#include "forward.hpp"
#include <vulkan/vulkan.hpp>

#include <functional>
#include <deque>
#include <memory>

namespace Engine {

// Forward declarations
class Task;

class TaskManager {
    friend class RenderEngine;
public:
    explicit TaskManager(VulkanDevice &device);
    std::unique_ptr<Task> createTask();

    /**
     * Submits a task to be executed.
     * This is a non-blocking submission.
     * Use Task::executeWhenComplete to run code after
     * the task is finished
     */
    vk::Fence submitTask(std::unique_ptr<Task> task);

private:
    void processActions();

    // Provided fields
    VulkanDevice &device;

    // State
    std::deque<std::unique_ptr<Task>> submittedTasks;
};

/**
 * A task allows you to execute vulkan commands once-off.
 * Useful for executing buffer transfers.
 */
class Task {
    friend class TaskManager;

public:
    /**
     * Executes some function in the context of the command buffer
     */
    void execute(const std::function<void(vk::CommandBuffer)> &);
    void addMemoryBarrier(
        const vk::PipelineStageFlags &fromStage, const vk::AccessFlags &fromAccess,
        const vk::PipelineStageFlags &toStage,
        const vk::AccessFlags &toAccess
    );

    /**
     * Adds some function to be executed after the task has successfully been run
     */
    void executeWhenComplete(const std::function<void()> &);

    void freeWhenDone(const std::shared_ptr<Buffer> &buffer);
    void freeWhenDone(std::unique_ptr<Buffer> &&buffer);
private:
    Task(vk::UniqueCommandBuffer, vk::UniqueFence, TaskManager &);

    void executeFinishCallbacks();

    // Provided
    vk::UniqueCommandBuffer commandBuffer;
    vk::UniqueFence submitFence;
    TaskManager &taskManager;

    // State
    std::vector<std::function<void()>> finishCallbacks;
    std::vector<std::shared_ptr<Buffer>> buffersToFree; // This will just be freed when the task is destroyed, which on be until submission is complete
};
}