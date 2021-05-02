#include "subsystem/light.hpp"
#include "vulkanutils.hpp"

#define DAY_SKY_BRIGHTNESS glm::vec3{0.8, 0.8, 0.8}

namespace Engine::Subsystem {

const SubsystemID<LightSubsystem> LightSubsystem::ID;
LightSubsystem::LightSubsystem()
{
    light = {
        DAY_SKY_BRIGHTNESS, // Sky light
        {0.3, 0.3, 0.3} // Ambient
    };
}

// Public API
void LightSubsystem::setTimeOfDay(float timePercent) {
    float sunAngle = fmodf(3.14159f * 2 * timePercent, 3.14159f * 2);

    // clamp((dot(up, sky.sunDirection) + 0.09) * 2.4, 0, 1);
    float intensity = std::min(std::max((sinf(sunAngle) + 0.09f) * 2.4f, 0.0f), 1.0f);
    light.globalSkyLight = DAY_SKY_BRIGHTNESS * intensity;
}

vk::DescriptorSetLayout LightSubsystem::layout() const {
    return descriptorLayout;
}

vk::DescriptorSet LightSubsystem::descriptor(uint32_t activeImage) const {
    return descriptorSets[activeImage];
}

// For engine use

void LightSubsystem::initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) {
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{
        { // light binding
            1, // binding
            vk::DescriptorType::eUniformBuffer,
            1, // count
            vk::ShaderStageFlagBits::eVertex
        }
    }};
    
    descriptorLayout = device.createDescriptorSetLayout({
        {}, vkUseArray(bindings)
    });

    bufferManager = &engine.getBufferManager();
}

void LightSubsystem::initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) {
    lightBuffers.resize(swapChainImages);
    for (uint32_t i = 0; i < swapChainImages; ++i) {
        lightBuffers[i] = std::move(engine.getBufferManager().aquire(
            sizeof(LightUBO),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryUsage::eCPUToGPU
        ));
    }

    // Descriptor pool for allocating the descriptors
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
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
        vk::DescriptorBufferInfo lightInfo(
            lightBuffers[imageIndex]->buffer(),
            0,
            sizeof(LightUBO)
        );

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {{
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
}

void LightSubsystem::cleanupResources(vk::Device device, _E::RenderEngine &engine) {
    device.destroyDescriptorSetLayout(descriptorLayout);
}

void LightSubsystem::cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) {
    lightBuffers.clear();
    device.destroyDescriptorPool(descriptorPool);
}

void LightSubsystem::prepareFrame(uint32_t activeImage) {
    // TODO: Copy lighting buffer to light UBO
    lightBuffers[activeImage]->copyIn(
        &light,
        0, // Offset
        sizeof(LightUBO)
    );

    lightBuffers[activeImage]->flush();
}

void LightSubsystem::writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) {
}

}