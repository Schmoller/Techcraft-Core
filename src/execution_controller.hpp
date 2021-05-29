#pragma once

#include "tech-core/forward.hpp"
#include <vulkan/vulkan.hpp>
#include <glm/fwd.hpp>

namespace Engine {

enum class ExecutionStage {
    FirstThing,
    Vertex,
    Geometry,
    Tessellation,
    Fragment,
    Compute,
    LastThing
};

enum class ResourceUsage {
    Read,
    Write,
};

enum class BindPoint {
    Uniform,
    Storage,
    Sampled
};

class ExecutionController {
public:
    ExecutionController(VulkanDevice &device, uint32_t chainSize);
    ~ExecutionController();

    vk::CommandBuffer acquireSecondaryGraphicsCommandBuffer();
    vk::CommandBuffer acquireSecondaryComputeCommandBuffer();

    void startRender(uint32_t imageIndex);
    void endRender();

    // Render pipeline
    void beginRenderPass(vk::RenderPass, vk::Framebuffer, vk::Extent2D, const glm::vec4 &clear);
    void addToRender(vk::CommandBuffer);
    void endRenderPass();

    // Compute pipeline
    void queueCompute(ComputeTask &);

    /**
     * Marks a resource as used within this frame. This will handle automatic memory barriers, layout transitions,
     * and queue transfer.
     *
     * This may be used at any time during construction of the command buffers.
     *
     * Todo: implementation note, this should add the resource to a collection of resources being used this frame. It should record the minimum execution stage and the maximum.
     * As we begin the frame we should build a command buffer that contains all of the memory barriers required to get all the resources into the correct state
     * we should also at the end after we have built all the other command buffers, add in the memory barriers required to release any resources for queue transfer
     * Finally we should also use this to do queue synchronization with semaphores
     *
     * @param image The image being used
     * @param where The stage that uses it
     * @param bindPoint What kind of location it is being bound to
     * @param usage The useage it will have
     */
    void useResource(
        const Image &image, ExecutionStage where, BindPoint bindPoint, ResourceUsage usage = ResourceUsage::Read
    );
    void useResource(
        const Buffer &buffer, ExecutionStage where, BindPoint bindPoint, ResourceUsage usage = ResourceUsage::Read
    );

    vk::CommandBuffer beginExecution(const ComputeTask &task);
private:
    // Provided
    VulkanDevice &device;

    // Owned resources
    struct {
        std::vector<vk::CommandBuffer> graphics;
        std::vector<vk::CommandBuffer> compute;
    } primaryCommandBuffers;
    struct {
        std::vector<vk::CommandBuffer> graphics;
        std::vector<vk::CommandBuffer> compute;
    } secondaryCommandBuffers;

    // Current primary command buffers
    vk::CommandBuffer currentGraphicsBuffer;
    vk::CommandBuffer currentComputeBuffer;
    uint32_t activeIndex { 0 };

    std::vector<ComputeTask *> queuedComputeTasks;

    void fillComputeBuffers();
};

}



