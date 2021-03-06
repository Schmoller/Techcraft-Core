#pragma once

// Forward declarations
namespace Engine::Subsystem {
class Subsystem;
template<typename T>
class SubsystemID;
}

#include "tech-core/forward.hpp"
#include "tech-core/common_includes.hpp"
#include <vk_mem_alloc.h>

namespace Engine::Subsystem {
namespace _E = Engine;

enum class SubsystemLayer {
    // A special layer which does not execute in a render pass.
    BeforePasses,
    Main,
    Overlay
};

/**
 * A subset of the scene.
 * This is an abstract class. Subclass to implement logic
 * 
 * Subclasses classes are required to have a public static id field.
 * 
 *     public:
 *     static const uint32_t id = ...;
 */
class Subsystem {
public:
    /**
     * Initialises anything which requires access to the window object
     * @param window The window object
     */
    virtual void initialiseWindow(GLFWwindow *window) {};

    /**
     * Initialise general resources
     */
    virtual void
    initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) = 0;
    /**
     * Initialise resources dependant on the swap chain.
     * This will be called any time the swap chain needs to be rebuilt
     */
    virtual void
    initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) = 0;
    /**
     * Clean up general resources
     */
    virtual void cleanupResources(vk::Device device, _E::RenderEngine &engine) = 0;
    /**
     * Clean up resources dependant on the swap chain.
     * This will be called any time the swap chain needs to be rebuilt
     */
    virtual void cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) = 0;
    /**
     * Fill the frame command buffers
     */
    virtual void writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) = 0;

    /**
     * Executed at the start of a frame
     */
    virtual void beginFrame() {};

    virtual void writeBarriers(vk::CommandBuffer commandBuffer) {};

    /**
     * Prepares for a frame to be executed
     */
    virtual void prepareFrame(uint32_t activeImage) {};

    /**
     * Handles any cleanup after a frame
     */
    virtual void afterFrame(uint32_t activeImage) {};

    virtual SubsystemLayer getLayer() const { return SubsystemLayer::Main; }
};

template<typename T>
class SubsystemID {
public:
    // SubsystemID(uint32_t id);
};

}