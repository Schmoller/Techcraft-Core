#pragma once

#include "tech-core/forward.hpp"
#include <vulkan/vulkan.hpp>

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
    explicit ExecutionController(VulkanDevice &device);
    ~ExecutionController();

    void beginFrame();
    void endFrame();

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
    vk::CommandBuffer commandBuffer;

    bool hasComputeThisFrame { false };

    void initializeCompute();
};

}



