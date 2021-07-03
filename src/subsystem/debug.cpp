#include "tech-core/subsystem/debug.hpp"
#include "tech-core/shapes/bounding_box.hpp"
#include "tech-core/engine.hpp"
#include "vulkanutils.hpp"
#include "internal/packaged/debugline_frag_glsl.h"
#include "internal/packaged/debugline_vert_glsl.h"

namespace Engine::Subsystem {
namespace _E = Engine;

// DebugSubsystem
const SubsystemID<DebugSubsystem> DebugSubsystem::ID;
DebugSubsystem *DebugSubsystem::inst;

DebugSubsystem *DebugSubsystem::instance() {
    return inst;
}

DebugSubsystem::DebugSubsystem() {
    inst = this;
}

void DebugSubsystem::debugDrawLine(const glm::vec3 &from, const glm::vec3 &to, uint32_t color) {
    DebugLinePC command = {
        glm::vec4(from, 1.0),
        glm::vec4(to, 1.0),
        {
            ((color & 0xFF0000) >> 16) / 255.0,
            ((color & 0x00FF00) >> 8) / 255.0,
            ((color & 0x0000FF) >> 0) / 255.0,
            ((color & 0x0000FF) >> 24) / 255.0
        }
    };

    debugDrawCmds.push_back(command);
}

void DebugSubsystem::debugDrawBox(const glm::vec3 &from, const glm::vec3 &to, uint32_t color) {
    // Vertices of box
    glm::vec3 minXminYminZ = { glm::min(from.x, to.x), glm::min(from.y, to.y), glm::min(from.z, to.z) };
    glm::vec3 maxXminYminZ = { glm::max(from.x, to.x), glm::min(from.y, to.y), glm::min(from.z, to.z) };
    glm::vec3 minXmaxYminZ = { glm::min(from.x, to.x), glm::max(from.y, to.y), glm::min(from.z, to.z) };
    glm::vec3 minXminYmaxZ = { glm::min(from.x, to.x), glm::min(from.y, to.y), glm::max(from.z, to.z) };
    glm::vec3 maxXmaxYminZ = { glm::max(from.x, to.x), glm::max(from.y, to.y), glm::min(from.z, to.z) };
    glm::vec3 minXmaxYmaxZ = { glm::min(from.x, to.x), glm::max(from.y, to.y), glm::max(from.z, to.z) };
    glm::vec3 maxXminYmaxZ = { glm::max(from.x, to.x), glm::min(from.y, to.y), glm::max(from.z, to.z) };
    glm::vec3 maxXmaxYmaxZ = { glm::max(from.x, to.x), glm::max(from.y, to.y), glm::max(from.z, to.z) };

    // Draw edges
    debugDrawLine(minXmaxYmaxZ, maxXmaxYmaxZ, color);
    debugDrawLine(maxXmaxYmaxZ, maxXmaxYminZ, color);
    debugDrawLine(maxXmaxYminZ, minXmaxYminZ, color);
    debugDrawLine(minXmaxYminZ, minXmaxYmaxZ, color);

    debugDrawLine(minXminYminZ, maxXminYminZ, color);
    debugDrawLine(maxXminYminZ, maxXminYmaxZ, color);
    debugDrawLine(maxXminYmaxZ, minXminYmaxZ, color);
    debugDrawLine(minXminYmaxZ, minXminYminZ, color);

    debugDrawLine(maxXmaxYminZ, maxXminYminZ, color);
    debugDrawLine(maxXmaxYmaxZ, maxXminYmaxZ, color);
    debugDrawLine(minXmaxYmaxZ, minXminYmaxZ, color);
    debugDrawLine(minXmaxYminZ, minXminYminZ, color);
}

void DebugSubsystem::debugDrawBox(const BoundingBox &box, uint32_t color) {
    debugDrawBox(
        { box.xMin, box.yMin, box.zMin },
        { box.xMax, box.yMax, box.zMax },
        color
    );
}

void
DebugSubsystem::initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) {
}

void
DebugSubsystem::initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) {
    pipeline = engine.createPipeline()
        .withVertexShader(DEBUGLINE_VERT_GLSL, DEBUGLINE_VERT_GLSL_SIZE)
        .withFragmentShader(DEBUGLINE_FRAG_GLSL, DEBUGLINE_FRAG_GLSL_SIZE)
        .withGeometryType(PipelineGeometryType::SegmentedLines)
        .withPushConstants<DebugLinePC>(vk::ShaderStageFlagBits::eVertex)
        .bindCamera(0, 0)
        .build();
}

void DebugSubsystem::cleanupResources(vk::Device device, _E::RenderEngine &engine) {
}

void DebugSubsystem::cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) {
    pipeline.reset();
}

void DebugSubsystem::writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) {
    if (debugDrawCmds.size() > 0) {
        pipeline->bind(commandBuffer, activeImage);

        for (auto command : debugDrawCmds) {
            pipeline->push(commandBuffer, vk::ShaderStageFlagBits::eVertex, command);
            commandBuffer.draw(2, 1, 0, 0);
        }
    }
}

void DebugSubsystem::afterFrame(uint32_t activeImage) {
    // Clear debug draw commands
    debugDrawCmds.clear();
}


}