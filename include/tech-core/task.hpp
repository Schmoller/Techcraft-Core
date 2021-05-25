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
    ~Task();

    /**
     * Executes some function in the context of the command buffer
     */
    void execute(std::function<void(vk::CommandBuffer)>);
    void addMemoryBarrier(
        vk::PipelineStageFlags fromStage, vk::AccessFlags fromAccess, vk::PipelineStageFlags toStage,
        vk::AccessFlags toAccess
    );

    /**
     * Adds some function to be executed after the task has successfully been run
     */
    void executeWhenComplete(std::function<void()>);

private:
    Task(vk::UniqueCommandBuffer, vk::UniqueFence, TaskManager &);

    void executeFinishCallbacks();

    // Provided
    vk::UniqueCommandBuffer commandBuffer;
    vk::UniqueFence submitFence;
    TaskManager &taskManager;

    // State
    std::vector<std::function<void()>> finishCallbacks;
};
}