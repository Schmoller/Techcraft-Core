#pragma once

// Forward declarations
namespace Engine::Subsystem {
    class Subsystem;
    template <typename T>
    class SubsystemID;
}

#include "tech-core/common_includes.hpp"
#include "tech-core/camera.hpp"
#include <vk_mem_alloc.h>
#include "tech-core/buffer.hpp"
#include "tech-core/texturemanager.hpp"
#include "tech-core/engine.hpp"

namespace Engine::Subsystem {
namespace _E = Engine;

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
     * Initialise general resources
     */
    virtual void initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) = 0;
    /**
     * Initialise resources dependant on the swap chain.
     * This will be called any time the swap chain needs to be rebuilt
     */
    virtual void initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) = 0;
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
     * Prepares for a frame to be executed
     */
    virtual void prepareFrame(uint32_t activeImage) {};
    /**
     * Handles any cleanup after a frame
     */
    virtual void afterFrame(uint32_t activeImage) {};
};

template <typename T>
class SubsystemID {
    public:
    // SubsystemID(uint32_t id);
};

}