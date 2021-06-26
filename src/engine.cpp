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
#include "tech-core/post_processing.hpp"
#include "tech-core/texture/manager.hpp"
#include "tech-core/texture/builder.hpp"
#include "tech-core/scene/scene.hpp"
#include "tech-core/material/manager.hpp"
#include "tech-core/material/builder.hpp"

#include "vulkanutils.hpp"
#include "imageutils.hpp"
#include "execution_controller.hpp"
#include "scene/render_planner.hpp"

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
    addSubsystem(Internal::RenderPlanner::ID);

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
    textureManager = std::make_unique<TextureManager>(*this, *device, physicalDevice);
    executionController = std::make_unique<ExecutionController>(*device, swapChain->size());

    createAttachments();
    createMainRenderPass();
    createOverlayRenderPass();
    createDepthResources();
    createFramebuffers();
    updateEffectPipelines();

    createUniformBuffers();
    materialManager = std::make_unique<MaterialManager>();
    fontManager = std::make_unique<FontManager>(
        *textureManager
    );
    guiManager = std::unique_ptr<Gui::GuiManager>(
        new Gui::GuiManager(
            device->device,
            *textureManager,
            *bufferManager,
            *taskManager,
            *fontManager,
            createPipeline(Subsystem::SubsystemLayer::Overlay),
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

    vk::Extent2D windowExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    swapChain->rebuild(windowExtent);

    createAttachments();
    createMainRenderPass();
    createOverlayRenderPass();
    guiManager->recreatePipeline(createPipeline(), swapChain->extent);
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createCommandBuffers();
    updateEffectPipelines();

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

void RenderEngine::createAttachments() {
    intermediateAttachments.resize(2);
    auto attachmentBuilder = createImage(swapChain->extent.width, swapChain->extent.height)
        .withMipLevels(1)
        .withFormat(swapChain->imageFormat)
        .withMemoryUsage(vk::MemoryUsage::eGPUOnly)
        .withUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment)
        .withImageTiling(vk::ImageTiling::eOptimal)
        .withSampleCount(vk::SampleCountFlagBits::e1);

    for (uint32_t i = 0; i < 2; ++i) {
        intermediateAttachments[i] = attachmentBuilder.build();
    }
}

void RenderEngine::createMainRenderPass() {
    std::array<vk::AttachmentDescription, 4> attachments;

    attachments[0] = {
        {},
        swapChain->imageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    attachments[1] = {
        {},
        swapChain->imageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    attachments[2] = {
        {},
        swapChain->imageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    attachments[3] = {
        {},
        findDepthFormat(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::AttachmentReference colorAttachmentFramebufferRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference colorAttachmentInt1Ref(1, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference colorAttachmentInt2Ref(2, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthAttachmentRef(3, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    std::array<vk::AttachmentReference, 2> inputAttachmentRef {
        vk::AttachmentReference { 1, vk::ImageLayout::eShaderReadOnlyOptimal },
        vk::AttachmentReference { 3, vk::ImageLayout::eShaderReadOnlyOptimal },
    };

    std::array<vk::AttachmentReference, 2> inputAttachmentRefAlt {
        vk::AttachmentReference { 2, vk::ImageLayout::eShaderReadOnlyOptimal },
        vk::AttachmentReference { 3, vk::ImageLayout::eShaderReadOnlyOptimal },
    };

    std::vector<vk::SubpassDescription> subpasses(effects.size() + 1);
    std::vector<vk::SubpassDependency> dependencies;
    for (uint32_t pass = 0; pass < effects.size() + 1; ++pass) {
        if (pass == 0) {
            subpasses[pass] = {
                {},
                vk::PipelineBindPoint::eGraphics,
                0, nullptr,
                1, nullptr,
                nullptr,
                &depthAttachmentRef
            };

            dependencies.emplace_back(
                vk::SubpassDependency {
                    VK_SUBPASS_EXTERNAL,
                    pass,
                    vk::PipelineStageFlagBits::eBottomOfPipe,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::AccessFlagBits::eMemoryRead,
                    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                    vk::DependencyFlagBits::eByRegion
                }
            );
//            dependencies.emplace_back(
//                vk::SubpassDependency {
//                    pass,
//                    VK_SUBPASS_EXTERNAL,
//                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
//                    vk::PipelineStageFlagBits::eBottomOfPipe,
//                    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
//                    vk::AccessFlagBits::eMemoryRead,
//                    vk::DependencyFlagBits::eByRegion
//                }
//            );
        } else {
            subpasses[pass] = {
                {},
                vk::PipelineBindPoint::eGraphics,
                2, nullptr,
                1, nullptr,
                nullptr,
                nullptr
            };

            if (pass > 1) {
                // Allow rewriting a previously used attachment
                dependencies.emplace_back(
                    vk::SubpassDependency {
                        pass - 1,
                        pass,
                        vk::PipelineStageFlagBits::eFragmentShader,
                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::AccessFlagBits::eShaderRead,
                        vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
                        vk::DependencyFlagBits::eByRegion
                    }
                );
            }

            // Allow reading the previous stages color attachment
            if (pass == 1) {
                dependencies.emplace_back(
                    vk::SubpassDependency {
                        pass - 1,
                        pass,
                        vk::PipelineStageFlagBits::eColorAttachmentOutput |
                            vk::PipelineStageFlagBits::eEarlyFragmentTests |
                            vk::PipelineStageFlagBits::eLateFragmentTests,
                        vk::PipelineStageFlagBits::eFragmentShader,
                        vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                        vk::AccessFlagBits::eInputAttachmentRead,
                        vk::DependencyFlagBits::eByRegion
                    }
                );
            } else {
                dependencies.emplace_back(
                    vk::SubpassDependency {
                        pass - 1,
                        pass,
                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::PipelineStageFlagBits::eFragmentShader,
                        vk::AccessFlagBits::eColorAttachmentWrite,
                        vk::AccessFlagBits::eShaderRead,
                        vk::DependencyFlagBits::eByRegion
                    }
                );
            }

            if (pass % 2 == 0) {
                subpasses[pass].setPInputAttachments(inputAttachmentRefAlt.data());
            } else {
                subpasses[pass].setPInputAttachments(inputAttachmentRef.data());
            }
        }

        if (pass == effects.size()) {
            // final pass output to frame buffer
            subpasses[pass].setPColorAttachments(&colorAttachmentFramebufferRef);
        } else {
            // Alternate between the two intermediate attachments
            if (pass % 2 == 0) {
                subpasses[pass].setPColorAttachments(&colorAttachmentInt1Ref);
            } else {
                subpasses[pass].setPColorAttachments(&colorAttachmentInt2Ref);
            }
        }
    }

    vk::RenderPassCreateInfo renderPassInfo(
        {},
        vkUseArray(attachments),
        vkUseArray(subpasses),
        vkUseArray(dependencies)
    );

    layerMain.renderPass = device->device.createRenderPass(renderPassInfo);
}

void RenderEngine::createOverlayRenderPass() {
    vk::AttachmentDescription colorAttachment(
        {},
        swapChain->imageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eLoad,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR
    );

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription depthAttachment(
        {},
        findDepthFormat(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &colorAttachmentRef,
        nullptr,
        nullptr
    );

    vk::SubpassDependency depColourWrite(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
    );

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    std::array<vk::SubpassDependency, 1> dependencies = {
        depColourWrite
    };

    vk::RenderPassCreateInfo renderPassInfo(
        {},
        vkUseArray(attachments),
        1, &subpass,
        vkUseArray(dependencies)
    );

    layerOverlay.renderPass = device->device.createRenderPass(renderPassInfo);
}

void RenderEngine::updateEffectPipelines() {
    uint32_t subpass = 1;
    for (auto &effect : effects) {
        effect->onSwapChainRecreate(layerMain.renderPass, swapChain->extent, subpass);
        if (subpass % 2 == 0) {
            effect->bindImage(0, 0, intermediateAttachments[1]);
        } else {
            effect->bindImage(0, 0, intermediateAttachments[0]);
        }
        effect->bindImage(0, 1, finalDepthAttachment);
        ++subpass;
    }
}

vk::ShaderModule RenderEngine::createShaderModule(const std::vector<char> &code) {
    vk::ShaderModuleCreateInfo createInfo(
        {}, static_cast<uint32_t>(code.size()), reinterpret_cast<const uint32_t *>(code.data())
    );

    return device->device.createShaderModule(createInfo);
}

void RenderEngine::createFramebuffers() {
    layerMain.framebuffers.resize(swapChain->size());
    layerOverlay.framebuffers.resize(swapChain->size());

    for (size_t i = 0; i < layerMain.framebuffers.size(); ++i) {
        std::array<vk::ImageView, 4> mainAttachments = {
            swapChain->imageViews[i],
            intermediateAttachments[0]->imageView(),
            intermediateAttachments[1]->imageView(),
            finalDepthAttachment->imageView()
        };

        vk::FramebufferCreateInfo mainFramebufferInfo(
            {},
            layerMain.renderPass,
            vkUseArray(mainAttachments),
            swapChain->extent.width,
            swapChain->extent.height,
            1
        );

        layerMain.framebuffers[i] = device->device.createFramebuffer(mainFramebufferInfo);

        std::array<vk::ImageView, 2> overlayAttachments = {
            swapChain->imageViews[i],
            finalDepthAttachment->imageView()
        };

        vk::FramebufferCreateInfo overlayFramebufferInfo(
            {},
            layerOverlay.renderPass,
            vkUseArray(overlayAttachments),
            swapChain->extent.width,
            swapChain->extent.height,
            1
        );

        layerOverlay.framebuffers[i] = device->device.createFramebuffer(overlayFramebufferInfo);
    }
}


void RenderEngine::createCommandBuffers() {
    guiCommandBuffer = executionController->acquireSecondaryGraphicsCommandBuffer();
    layerMain.commandBuffer = executionController->acquireSecondaryGraphicsCommandBuffer();
    layerOverlay.commandBuffer = executionController->acquireSecondaryGraphicsCommandBuffer();
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
    auto builder = createImage(swapChain->extent.width, swapChain->extent.height)
        .withFormat(depthFormat)
        .withImageTiling(vk::ImageTiling::eOptimal)
        .withUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment)
        .withMemoryUsage(vk::MemoryUsage::eGPUOnly)
        .withSampleCount(vk::SampleCountFlagBits::e1)
        .withMipLevels(1);

    finalDepthAttachment = builder.build();

    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
    finalDepthAttachment->transition(commandBuffer, vk::ImageLayout::eDepthStencilAttachmentOptimal);
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

void RenderEngine::fillFrameCommands(
    vk::CommandBuffer buffer, uint32_t currentImage, Subsystem::SubsystemLayer layer
) {
    for (auto &subsystem : orderedSubsystems) {
        if (subsystem->getLayer() == layer) {
            subsystem->writeFrameCommands(buffer, currentImage);
        }
    }
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

    // Main layer
    executionController->beginRenderPass(
        layerMain.renderPass, layerMain.framebuffers[imageIndex], swapChain->extent, { 0, 0, 0, 1 }, 2
    );

    vk::CommandBufferInheritanceInfo mainCbInheritance(
        layerMain.renderPass,
        0,
        layerMain.framebuffers[imageIndex]
    );

    vk::CommandBufferBeginInfo renderBeginInfo(
        vk::CommandBufferUsageFlagBits::eRenderPassContinue,
        &mainCbInheritance
    );
    layerMain.commandBuffer.begin(renderBeginInfo);

    fillFrameCommands(layerMain.commandBuffer, imageIndex, Subsystem::SubsystemLayer::Main);

    layerMain.commandBuffer.end();
    executionController->addToRender(layerMain.commandBuffer);

    // Do post processing effects
    uint32_t pass = 1;
    for (auto &effect : effects) {
        vk::CommandBufferInheritanceInfo cbInheritance(
            layerMain.renderPass,
            pass,
            layerMain.framebuffers[imageIndex]
        );

        vk::CommandBufferBeginInfo effectBeginInfo(
            vk::CommandBufferUsageFlagBits::eRenderPassContinue,
            &cbInheritance
        );

        effect->getCommandBuffer().begin(effectBeginInfo);
        executionController->nextSubpass();
        effect->fillFrameCommands();
        effect->getCommandBuffer().end();
        executionController->addToRender(effect->getCommandBuffer());
        ++pass;
    }

    executionController->endRenderPass();

    // Overlay layer
    vk::CommandBufferInheritanceInfo overlayCbInheritance(
        layerOverlay.renderPass,
        0,
        layerOverlay.framebuffers[imageIndex]
    );

    executionController->beginRenderPass(
        layerOverlay.renderPass, layerOverlay.framebuffers[imageIndex], swapChain->extent, { 0, 0, 0, 1 }
    );

    vk::CommandBufferBeginInfo overlayBeginInfo(
        vk::CommandBufferUsageFlagBits::eRenderPassContinue,
        &overlayCbInheritance
    );
    layerOverlay.commandBuffer.begin(overlayBeginInfo);

    guiManager->render(guiCommandBuffer, overlayCbInheritance);
    fillFrameCommands(layerOverlay.commandBuffer, imageIndex, Subsystem::SubsystemLayer::Overlay);

    layerOverlay.commandBuffer.end();
    executionController->addToRender(layerOverlay.commandBuffer);
    executionController->addToRender(guiCommandBuffer);
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

    finalDepthAttachment.reset();

    for (auto &framebuffer : layerMain.framebuffers) {
        device->device.destroyFramebuffer(framebuffer);
    }
    for (auto &framebuffer : layerOverlay.framebuffers) {
        device->device.destroyFramebuffer(framebuffer);
    }

    for (size_t i = 0; i < uniformBuffers.size(); ++i) {
        uniformBuffers[i].destroy();
    }

    device->device.destroyRenderPass(layerMain.renderPass);
    device->device.destroyRenderPass(layerOverlay.renderPass);
}

void RenderEngine::cleanup() {
    device->waitIdle();

    cout << "cleanup" << endl;
    cleanupSwapChain();

    for (auto &subsystem : orderedSubsystems) {
        subsystem->cleanupResources(device->device, *this);
    }

    effects.clear();

    meshes.clear();
    guiManager.reset();
    materialManager.reset();
    textureManager.reset();

    // Release all remaining buffers
    bufferManager->processActions();
    taskManager.reset();
    executionController.reset();

    intermediateAttachments.clear();
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
    return textureManager->add(name);
}

const Texture *RenderEngine::getTexture(const std::string &name) {
    return textureManager->get(name);
}

const Texture *RenderEngine::getTexture(const char *name) {
    return getTexture(std::string(name));
}

void RenderEngine::destroyTexture(const std::string &name) {
    textureManager->remove(name);
}

void RenderEngine::destroyTexture(const char *name) {
    destroyTexture(std::string(name));
}

ImageBuilder RenderEngine::createImage(uint32_t width, uint32_t height) {
    return ImageBuilder(*device, width, height);
}

ImageBuilder RenderEngine::createImageArray(uint32_t width, uint32_t height, uint32_t count) {
    return ImageBuilder(*device, width, height, count);
}

// ==============================================
//  Material Methods
// ==============================================
MaterialBuilder RenderEngine::createMaterial(const std::string &name) {
    return materialManager->add(name);
}

Material *RenderEngine::getMaterial(const std::string &name) {
    return materialManager->get(name);
}

Material *RenderEngine::getMaterial(const char *name) {
    return getMaterial(std::string(name));
}

void RenderEngine::destroyMaterial(const std::string &name) {
    materialManager->remove(name);
}

void RenderEngine::destroyMaterial(const char *name) {
    destroyMaterial(std::string(name));
}

PipelineBuilder RenderEngine::createPipeline(Subsystem::SubsystemLayer layer) {
    vk::RenderPass renderPass;
    if (layer == Subsystem::SubsystemLayer::Main) {
        renderPass = layerMain.renderPass;
    } else {
        renderPass = layerOverlay.renderPass;
    }
    return PipelineBuilder(
        *this,
        device->device,
        renderPass,
        swapChain->extent,
        layerMain.framebuffers.size()
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

EffectBuilder RenderEngine::createEffect(const std::string &name) {
    PipelineBuilder builder(
        *this, device->device, layerMain.renderPass, swapChain->extent, layerMain.framebuffers.size());
    builder
        .withInputAttachment(0, 0)
        .withInputAttachment(0, 1);

    return EffectBuilder(
        name,
        *this,
        builder
    );
}

void RenderEngine::addEffect(const std::shared_ptr<Effect> &effect) {
    effects.push_back(effect);
    effectsByName[effect->getName()] = effect;
    auto buffer = executionController->acquireSecondaryGraphicsCommandBuffer();
    effect->applyCommandBuffer(buffer);

    if (layerMain.renderPass) {
        recreateSwapChain();
    }
}

std::shared_ptr<Effect> RenderEngine::getEffect(const std::string &name) {
    auto it = effectsByName.find(name);
    if (it == effectsByName.end()) {
        return {};
    }
    return it->second;
}

void RenderEngine::removeEffect(const std::string &name) {
    // TODO: remove
}

void RenderEngine::setScene(const std::shared_ptr<Scene> &scene) {
    if (currentScene) {
        currentScene->onSetInactive({});
    }
    currentScene = scene;
    currentScene->onSetActive({}, getSubsystem(Internal::RenderPlanner::ID));
}

}
