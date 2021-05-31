#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <set>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <unordered_map>

#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#include "tech-core/engine.hpp"
#include "tech-core/mesh.hpp"
#include "tech-core/buffer.hpp"
#include "tech-core/image.hpp"
#include "tech-core/camera.hpp"
#include "tech-core/device.hpp"
#include "tech-core/swapchain.hpp"
#include "tech-core/pipeline.hpp"
#include "tech-core/font.hpp"
#include "tech-core/compute.hpp"

#include "vulkanutils.hpp"
#include "imageutils.hpp"
#include "execution_controller.hpp"

const int WIDTH = 1920;
const int HEIGHT = 1080;

using std::cout, std::endl;

#ifndef NDEBUG
#define ENABLE_VALIDATION_LAYERS
#endif

const std::vector<const char *> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace Engine {

RenderEngine::RenderEngine() {
}

RenderEngine::~RenderEngine() {
    cleanup();
}

void RenderEngine::initialize(const std::string_view &title) {
    initWindow(title);
    initVulkan();
}

void RenderEngine::initWindow(const std::string_view &title) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, title.data(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    inputManager.initialize(window);
}

void RenderEngine::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<RenderEngine *>(
        glfwGetWindowUserPointer(window)
    );
    app->framebufferResized = true;
}

void RenderEngine::initVulkan() {
    createInstance();
    createSurface();

    // Find a suitable GPU
    auto gpus = instance.enumeratePhysicalDevices();

    if (gpus.size() == 0) {
        throw std::runtime_error("No GPUs available");
    }

    for (auto &offeredGPU : gpus) {
        try {
            this->device = std::make_unique<VulkanDevice>(offeredGPU, this->instance, surface);
            physicalDevice = offeredGPU;
            break;
        } catch (DeviceNotSuitable &error) {
            continue;
        }
    }

    if (!physicalDevice) {
        throw std::runtime_error("No suitable GPU found");
    }

    // Init the swap chain
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D windowExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    swapChain = std::make_unique<SwapChain>(physicalDevice, *device, surface, windowExtent);

    // Other resources
    bufferManager = std::make_unique<BufferManager>(*device);
    taskManager = std::make_unique<TaskManager>(*device);
    executionController = std::make_unique<ExecutionController>(*device, swapChain->size());

    createRenderPass();
    createDescriptorSetLayout();
    createDepthResources();
    createFramebuffers();

    createUniformBuffers();
    textureManager.initialize(
        device->device,
        device->allocator,
        device->graphicsPool,
        device->graphicsQueue.queue,
        textureDescriptorLayout
    );
    loadPlaceholders();
    materialManager.initialize(
        device->device,
        &textureManager
    );
    fontManager = std::make_unique<FontManager>(
        textureManager
    );
    guiManager = std::unique_ptr<Gui::GuiManager>(
        new Gui::GuiManager(
            device->device,
            textureManager,
            *bufferManager,
            *taskManager,
            *fontManager,
            createPipeline(),
            swapChain->extent
        ));

    createCommandBuffers();

    for (auto &subsystem : orderedSubsystems) {
        subsystem->initialiseWindow(window);
        subsystem->initialiseResources(device->device, physicalDevice, *this);
        subsystem->initialiseSwapChainResources(device->device, *this, swapChain->images.size());
    }
}

void RenderEngine::recreateSwapChain() {
    cout << "Recreating swap chain" << endl;

    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    if (camera) {
        camera->setAspectRatio(swapChain->extent.width / (float) swapChain->extent.height);
    }

    device->waitIdle();

    cleanupSwapChain();
    textureManager.onSwapChainRecreate();

    vk::Extent2D windowExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    swapChain->rebuild(windowExtent);
    createRenderPass();
    guiManager->recreatePipeline(createPipeline(), swapChain->extent);
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createCommandBuffers();

    for (auto &subsystem : orderedSubsystems) {
        subsystem->initialiseSwapChainResources(device->device, *this, swapChain->images.size());
    }

    framebufferResized = false;
}

void RenderEngine::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }
}

void RenderEngine::createInstance() {
#ifdef ENABLE_VALIDATION_LAYERS
    if (!checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers are not available but requested");
    }
#endif

    vk::ApplicationInfo appInfo(
        "Hello Triangle",
        VK_MAKE_VERSION(1, 0, 0),
        "No Engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0
    );

    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo(&appInfo);

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.setEnabledExtensionCount(glfwExtensionCount);
    createInfo.setPpEnabledExtensionNames(glfwExtensions);

#ifdef ENABLE_VALIDATION_LAYERS
    createInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()));
    createInfo.setPpEnabledLayerNames(validationLayers.data());
#else
    createInfo.setEnabledLayerCount(0);
#endif

    instance = vk::createInstance(createInfo);

    printExtensions();
}

void RenderEngine::createRenderPass() {
    vk::AttachmentDescription colorAttachment(
        {},
        swapChain->imageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    );

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription depthAttachment(
        {},
        findDepthFormat(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &colorAttachmentRef,
        nullptr,
        &depthAttachmentRef
    );

    vk::SubpassDependency depColourWrite(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
    );

    // Allow memory barriers within the render pass
    vk::SubpassDependency depVertexBarrier(
        0,
        0,
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eVertexInput,
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eIndexRead,
        vk::DependencyFlagBits::eDeviceGroup
    );

    vk::SubpassDependency depWriteToRead(
        0,
        0,
        vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::AccessFlagBits::eShaderWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::DependencyFlagBits::eDeviceGroup
    );

    vk::SubpassDependency depReadToWrite(
        0,
        0,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::AccessFlagBits::eShaderRead,
        vk::AccessFlagBits::eShaderWrite,
        vk::DependencyFlagBits::eDeviceGroup
    );

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    std::array<vk::SubpassDependency, 4> dependencies = {
        depColourWrite, depVertexBarrier, depWriteToRead, depReadToWrite
    };

    vk::RenderPassCreateInfo renderPassInfo(
        {},
        vkUseArray(attachments),
        1, &subpass,
        vkUseArray(dependencies)
    );

    renderPass = device->device.createRenderPass(renderPassInfo);
}

void RenderEngine::createDescriptorSetLayout() {
    // Create the texture descriptor layout

    vk::DescriptorSetLayoutBinding samplerBinding(
        2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment
    );

    std::array<vk::DescriptorSetLayoutBinding, 1> textureBinding = { samplerBinding };

    vk::DescriptorSetLayoutCreateInfo textureLayoutInfo(
        {}, 1, textureBinding.data()
    );

    textureDescriptorLayout = device->device.createDescriptorSetLayout(textureLayoutInfo);
}

vk::ShaderModule RenderEngine::createShaderModule(const std::vector<char> &code) {
    vk::ShaderModuleCreateInfo createInfo(
        {}, static_cast<uint32_t>(code.size()), reinterpret_cast<const uint32_t *>(code.data())
    );

    return device->device.createShaderModule(createInfo);
}

void RenderEngine::createFramebuffers() {
    swapChainFramebuffers.resize(swapChain->size());

    for (size_t i = 0; i < swapChainFramebuffers.size(); ++i) {
        std::array<vk::ImageView, 2> attachments = {
            swapChain->imageViews[i],
            static_cast<vk::ImageView>(depthImage.imageView())
        };

        vk::FramebufferCreateInfo framebufferInfo(
            {},
            renderPass,
            2, attachments.data(),
            swapChain->extent.width,
            swapChain->extent.height,
            1
        );

        swapChainFramebuffers[i] = device->device.createFramebuffer(framebufferInfo);
    }
}


void RenderEngine::createCommandBuffers() {
    guiCommandBuffer = executionController->acquireSecondaryGraphicsCommandBuffer();
    renderCommandBuffer = executionController->acquireSecondaryGraphicsCommandBuffer();
}

void RenderEngine::createUniformBuffers() {
    uniformBuffers.resize(swapChain->size());

    VkDeviceSize bufferSize = sizeof(CameraUBO);

    for (size_t i = 0; i < swapChain->size(); ++i) {
        uniformBuffers[i].allocate(
            device->allocator,
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryUsage::eCPUToGPU
        );
    }
}

vk::CommandBuffer RenderEngine::beginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo(
        device->graphicsPool,
        vk::CommandBufferLevel::ePrimary,
        1
    );

    vk::CommandBuffer commandBuffer = device->device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void RenderEngine::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo(
        0, nullptr,
        nullptr,
        1, &commandBuffer
    );

    device->graphicsQueue.queue.submit(1, &submitInfo, vk::Fence());
    device->graphicsQueue.queue.waitIdle();

    device->device.freeCommandBuffers(device->graphicsPool, 1, &commandBuffer);
}

vk::Format RenderEngine::findSupportedFormat(
    const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features
) {
    for (auto format : candidates) {
        vk::FormatProperties properties = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format");
}

vk::Format RenderEngine::findDepthFormat() {
    return findSupportedFormat(
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

void RenderEngine::createDepthResources() {
    auto depthFormat = findDepthFormat();
    depthImage.allocate(
        device->allocator,
        device->device,
        swapChain->extent.width,
        swapChain->extent.height,
        depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        VMA_MEMORY_USAGE_GPU_ONLY,
        vk::SampleCountFlagBits::e1,
        1
    );

    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
    depthImage.transition(commandBuffer, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    endSingleTimeCommands(commandBuffer);
}

void RenderEngine::printExtensions() {
    auto extensions = vk::enumerateInstanceExtensionProperties();

    std::cout << "Extensions:" << std::endl;

    for (const auto &extension : extensions) {
        std::cout << "\t " << extension.extensionName << std::endl;
    }
}

bool RenderEngine::beginFrame() {
    if (glfwWindowShouldClose(window)) {
        return false;
    }

    bufferManager->processActions();
    taskManager->processActions();
    inputManager.updateStates();
    glfwPollEvents();

    for (auto &subsystem : orderedSubsystems) {
        subsystem->beginFrame();
    }

    return true;
}

void RenderEngine::render() {
    guiManager->update();
    drawFrame();
}

void RenderEngine::fillFrameCommands(vk::CommandBufferInheritanceInfo &cbInheritance, uint32_t currentImage) {
    vk::CommandBufferBeginInfo renderBeginInfo(
        vk::CommandBufferUsageFlagBits::eRenderPassContinue,
        &cbInheritance
    );
    renderCommandBuffer.begin(renderBeginInfo);

    for (auto &subsystem : orderedSubsystems) {
        subsystem->writeFrameCommands(renderCommandBuffer, currentImage);
    }

    renderCommandBuffer.end();

    executionController->addToRender(renderCommandBuffer);
    executionController->addToRender(guiCommandBuffer);
}

void RenderEngine::drawFrame() {
    device->device.waitForFences(1, &device->renderReady, VK_TRUE, std::numeric_limits<uint64_t>::max());
    device->device.waitForFences(1, &device->computeReady, VK_TRUE, std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex;
    try {
        imageIndex = device->device.acquireNextImageKHR(
            swapChain->swapChain, std::numeric_limits<uint64_t>::max(), device->presentFinished, vk::Fence()).value;
    } catch (vk::OutOfDateKHRError const &e) {
        recreateSwapChain();
        return;
    }

    executionController->startRender(imageIndex);

    for (auto &subsystem : orderedSubsystems) {
        subsystem->prepareFrame(imageIndex);
        executionController->addBarriers(*subsystem);
    }
    updateUniformBuffer(imageIndex);

    executionController->beginRenderPass(
        renderPass, swapChainFramebuffers[imageIndex], swapChain->extent, { 0, 0, 0, 1 }
    );

    vk::CommandBufferInheritanceInfo cbInheritance(
        renderPass,
        0,
        swapChainFramebuffers[imageIndex]
    );

    guiManager->render(guiCommandBuffer, cbInheritance);

    fillFrameCommands(cbInheritance, imageIndex);

    executionController->endRenderPass();
    executionController->endRender();

    vk::PresentInfoKHR presentInfo(
        1, &device->renderFinished,
        1, &swapChain->swapChain,
        &imageIndex
    );

    bool needsRecreateSwapChain = false;
    vk::Result result;
    try {
        result = device->presentQueue.queue.presentKHR(presentInfo);
    } catch (vk::OutOfDateKHRError const &e) {
        result = vk::Result::eSuboptimalKHR;
    }

    if (framebufferResized) {
        result = vk::Result::eSuboptimalKHR;
    }

    switch (result) {
        case vk::Result::eSuboptimalKHR:
            needsRecreateSwapChain = true;
            break;
        case vk::Result::eSuccess:
            // Do nothing
            break;
        default:
            throw std::runtime_error("failed to acquire swap chain image!");
    }

    for (auto &subsystem : orderedSubsystems) {
        subsystem->afterFrame(imageIndex);
    }

    if (needsRecreateSwapChain) {
        recreateSwapChain();
    }
}

void RenderEngine::updateUniformBuffer(uint32_t currentImage) {
    if (!camera) {
        return;
    }

    uniformBuffers[currentImage].copyIn(camera->getUBO(), sizeof(CameraUBO));
}

void RenderEngine::cleanupSwapChain() {
    cout << "cleanupSwapChain" << endl;

    for (auto &subsystem : orderedSubsystems) {
        subsystem->cleanupSwapChainResources(device->device, *this);
    }

    depthImage.destroy();

    for (auto &framebuffer : swapChainFramebuffers) {
        device->device.destroyFramebuffer(framebuffer);
    }

    for (size_t i = 0; i < uniformBuffers.size(); ++i) {
        uniformBuffers[i].destroy();
    }

    device->device.destroyRenderPass(renderPass);
}

void RenderEngine::cleanup() {
    device->waitIdle();

    cout << "cleanup" << endl;
    cleanupSwapChain();

    for (auto &subsystem : orderedSubsystems) {
        subsystem->cleanupResources(device->device, *this);
    }

    meshes.clear();
    guiManager.reset();
    materialManager.destroy();
    textureManager.destroy();

    // Release all remaining buffers
    bufferManager->processActions();
    taskManager.reset();
    device->device.destroyDescriptorSetLayout(textureDescriptorLayout);
    executionController.reset();

    swapChain->cleanup();
    swapChain.reset();
    device.reset();
    instance.destroySurfaceKHR(surface);
    instance.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    window = nullptr;
}

// ==============================================
//  Camera Methods
// ==============================================
void RenderEngine::setCamera(Camera &camera) {
    this->camera = &camera;
    camera.setAspectRatio(swapChain->extent.width / (float) swapChain->extent.height);
}

const Camera *RenderEngine::getCamera() const {
    return camera;
}

// ==============================================
//  Mesh Methods
// ==============================================
void RenderEngine::removeMesh(const std::string &name) {
    auto it = meshes.find(name);

    if (it == meshes.end()) {
        return;
    }

    // TODO: We need to ensure that the meshes buffer is only freed after the next frame
    // it->second->

    meshes.erase(it);
}

Mesh *RenderEngine::getMesh(const std::string &name) {
    if (meshes.count(name) > 0) {
        return meshes[name].get();
    }
    return nullptr;
}

// ==============================================
//  Texture Methods
// ==============================================
TextureBuilder RenderEngine::createTexture(const std::string &name) {
    return textureManager.createTexture(name);
}

Texture *RenderEngine::getTexture(const std::string &name) {
    return textureManager.getTexture(name);
}

Texture *RenderEngine::getTexture(const char *name) {
    return getTexture(std::string(name));
}

void RenderEngine::destroyTexture(const std::string &name) {
    textureManager.destroyTexture(name);
}

void RenderEngine::destroyTexture(const char *name) {
    destroyTexture(std::string(name));
}

ImageBuilder RenderEngine::createImage(uint32_t width, uint32_t height) {
    return ImageBuilder(*device, width, height);
}

void RenderEngine::loadPlaceholders() {
    VkDeviceSize imageSize = PLACEHOLDER_TEXTURE_SIZE * PLACEHOLDER_TEXTURE_SIZE;

    uint32_t *pixels = new uint32_t[imageSize];
    generateErrorPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels);

    textureManager.createTexture("internal.error")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .build();

    generateSolidPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels, 0x00000000);

    textureManager.createTexture("internal.loading")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .build();

    generateSolidPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels, 0xFFFFFFFF);

    textureManager.createTexture("internal.white")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .build();

    delete[] pixels;
}

// ==============================================
//  Material Methods
// ==============================================
Material *RenderEngine::createMaterial(const MaterialCreateInfo &createInfo) {
    return materialManager.addMaterial(createInfo);
}

Material *RenderEngine::getMaterial(const std::string &name) {
    return materialManager.getMaterial(name);
}

Material *RenderEngine::getMaterial(const char *name) {
    return getMaterial(std::string(name));
}

void RenderEngine::destroyMaterial(const std::string &name) {
    materialManager.destroyMaterial(name);
}

void RenderEngine::destroyMaterial(const char *name) {
    destroyMaterial(std::string(name));
}

PipelineBuilder RenderEngine::createPipeline() {
    return PipelineBuilder(
        *this,
        device->device,
        renderPass,
        swapChain->extent,
        swapChainFramebuffers.size()
    );
}

ComputeTaskBuilder RenderEngine::createComputeTask() {
    return ComputeTaskBuilder(device->device, *executionController);
}


// ==============================================
//  Utilities
// ==============================================

vk::DescriptorBufferInfo RenderEngine::getCameraDBI(uint32_t imageIndex) {
    return vk::DescriptorBufferInfo(
        uniformBuffers[imageIndex].buffer(),
        0,
        sizeof(CameraUBO)
    );
}

Gui::Rect RenderEngine::getScreenBounds() {
    return {
        { 0, 0 },
        { swapChain->extent.width, swapChain->extent.height }
    };
}

}