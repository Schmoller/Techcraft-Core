#include "subsystem/sky.hpp"
#include "vulkanutils.hpp"

#include <iostream>

namespace Engine::Subsystem {
namespace _E = Engine;

// SkySubsystem
const SubsystemID<SkySubsystem> SkySubsystem::ID;

SkySubsystem::SkySubsystem() {
    sky = {
        0.01433f,// sunAngularDiameter
        {1, 0, 0},// sunAngle
    };
}

void SkySubsystem::setTimeOfDay(float timePercent) {
    float sunAngle = fmodf(3.14159f * 2 * timePercent, 3.14159f * 2);

    sky.sunDirection = {cos(sunAngle), 0, sin(sunAngle)};
}

void SkySubsystem::initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) {
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{
        { // Camera binding
            0, // binding
            vk::DescriptorType::eUniformBuffer,
            1, // count
            vk::ShaderStageFlagBits::eFragment
        },
        { // sky binding
            1, // binding
            vk::DescriptorType::eUniformBuffer,
            1, // count
            vk::ShaderStageFlagBits::eFragment
        }
    }};
    
    descriptorLayout = device.createDescriptorSetLayout({
        {}, vkUseArray(bindings)
    });
}

void SkySubsystem::initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) {
    skyBuffers.resize(swapChainImages);
    for (uint32_t i = 0; i < swapChainImages; ++i) {
        skyBuffers[i] = std::move(engine.getBufferManager().aquire(
            sizeof(SkyUBO),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryUsage::eCPUToGPU
        ));
    }

    // Descriptor pool for allocating the descriptors
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {
            vk::DescriptorType::eUniformBuffer,
            swapChainImages
        },
        {
            vk::DescriptorType::eUniformBuffer,
            swapChainImages
        }
    }};

    descriptorPool = device.createDescriptorPool({
        {},
        swapChainImages,
        vkUseArray(poolSizes)
    });

    // Descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(swapChainImages, descriptorLayout);

    descriptorSets = device.allocateDescriptorSets({
        descriptorPool,
        vkUseArray(layouts)
    });

    // Assign buffers to DS'
    for (uint32_t imageIndex = 0; imageIndex < swapChainImages; ++imageIndex) {
        auto cameraUbo = engine.getCameraDBI(imageIndex);

        vk::DescriptorBufferInfo lightInfo(
            skyBuffers[imageIndex]->buffer(),
            0,
            sizeof(SkyUBO)
        );

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {{
            { // Camera UBO
                descriptorSets[imageIndex],
                0, // Binding
                0, // Array element
                1, // Count
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &cameraUbo
            },
            { // Lighting UBO
                descriptorSets[imageIndex],
                1, // Binding
                0, // Array element
                1, // Count
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &lightInfo
            }
        }};

        device.updateDescriptorSets(descriptorWrites, {});
    }

    pipeline = engine.createPipeline()
        .withVertexShader("assets/shaders/fullscreen-vert.spv")
        .withFragmentShader("assets/shaders/sky-frag.spv")
        .withGeometryType(PipelineGeometryType::Polygons)
        .withDescriptorSet(descriptorLayout)
        .withoutDepthTest()
        .withoutDepthWrite()
        .build();
}

void SkySubsystem::cleanupResources(vk::Device device, _E::RenderEngine &engine) {
    device.destroyDescriptorSetLayout(descriptorLayout);
}

void SkySubsystem::cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) {
    pipeline.reset();
    for (auto &buffer : skyBuffers) {
        buffer.reset();
    }
    device.destroyDescriptorPool(descriptorPool);
}

void SkySubsystem::prepareFrame(uint32_t activeImage) {
    skyBuffers[activeImage]->copyIn(
        &sky,
        0, // Offset
        sizeof(SkyUBO)
    );

    skyBuffers[activeImage]->flush();
}

void SkySubsystem::writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) {
    pipeline->bind(commandBuffer);
    pipeline->bindDescriptorSets(commandBuffer, 0, 1, &descriptorSets[activeImage], 0, nullptr);

    commandBuffer.draw(6, 1, 0, 0);
}

}