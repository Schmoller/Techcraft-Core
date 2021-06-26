#include "tech-core/subsystem/imgui.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/task.hpp"
#include "tech-core/pipeline.hpp"
#include "tech-core/image.hpp"
#include "tech-core/texture/common.hpp"
#include "tech-core/texture/manager.hpp"
#include "vulkanutils.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <glm/glm.hpp>
#include <iostream>

namespace Engine::Subsystem {
const SubsystemID<ImGuiSubsystem> ImGuiSubsystem::ID;

struct ImGuiPushConstant {
    alignas(8) glm::vec2 scale;
    alignas(8) glm::vec2 translate;
};

void ImGuiSubsystem::initialiseWindow(GLFWwindow *window) {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window, true);
}

void ImGuiSubsystem::initialiseResources(
    vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine
) {
    this->engine = &engine;
    this->device = device;

    // create font sampler
    fontSampler = device.createSampler(
        vk::SamplerCreateInfo(
            {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear
        )
            .setMinLod(-1000)
            .setMaxLod(1000)
            .setMaxAnisotropy(1.0f)
    );

    setupFont(device);
}

void ImGuiSubsystem::initialiseSwapChainResources(
    vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages
) {
    auto builder = engine.createPipeline(SubsystemLayer::Overlay)
        .withVertexBindingDescription(
            {
                0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex
            }
        )
        .withVertexAttributeDescription(
            {
                0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)
            }
        )
        .withVertexAttributeDescription(
            {
                1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)
            }
        )
        .withVertexAttributeDescription(
            {
                2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)
            }
        )
        .withoutFaceCulling()
        .withAlpha()
        .withDynamicState(vk::DynamicState::eViewport)
        .withDynamicState(vk::DynamicState::eScissor)
        .withoutDepthWrite()
        .withoutDepthTest()
        .bindSampledImagePoolImmutable(0, 0, MaxTexturesPerFrame, fontSampler)
        .withPushConstants<ImGuiPushConstant>(vk::ShaderStageFlagBits::eVertex);

    pipeline = builder
        .withVertexShader("assets/shaders/engine/imgui/vertex.spv")
        .withFragmentShader("assets/shaders/engine/imgui/fragment_plain.spv")
        .build();

    vertexBuffers.resize(swapChainImages);
}

void ImGuiSubsystem::cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) {
    pipeline.reset();
}

void ImGuiSubsystem::cleanupResources(vk::Device device, _E::RenderEngine &engine) {
    fontImage.reset();
    device.destroy(fontSampler);

    vertexBuffers.clear();
    vertexBuffers.shrink_to_fit();

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiSubsystem::writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) {
    int fbWidth = (int) (drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fbHeight = (int) (drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (fbWidth <= 0 || fbHeight <= 0)
        return;

    auto &buffers = vertexBuffers[activeImage];

    transferVertexInformation(drawData, commandBuffer, buffers);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int vertexOffset = 0;
    int indexOffset = 0;
    uint32_t lastBound = 0xFFFFFFFF;
    uint32_t lastSlot = 0xFFFFFFFF;

    Pipeline *currentPipeline { nullptr };

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList *commandList = drawData->CmdLists[n];
        for (int commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; commandIndex++) {
            const ImDrawCmd *command = &commandList->CmdBuffer[commandIndex];

            Image* image;
            if (Image::isImage(command->TextureId, device)) {
                image = reinterpret_cast<Image *>(command->TextureId);
            } else {
                image = reinterpret_cast<const Texture *>(command->TextureId)->getImage().get();
            }

            auto it = imagePoolMapping.find(image);
            if (it != imagePoolMapping.end()) {
                if (currentPipeline != pipeline.get()) {
                    currentPipeline = pipeline.get();

                    setupFrame(drawData, pipeline.get(), commandBuffer, buffers, fbWidth, fbHeight);
                    lastBound = 0xFFFFFFFF;
                    lastSlot = 0xFFFFFFFF;
                }

                if (it->second != lastBound) {
                    currentPipeline->bindPoolImage(commandBuffer, 0, 0, it->second);
                    lastBound = it->second;
                }
            } else {
                std::cerr << "Unable to find pool mapping for image " << image << std::endl;
                continue;
            }

            if (command->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (command->UserCallback == ImDrawCallback_ResetRenderState) {
                    setupFrame(drawData, nullptr, commandBuffer, buffers, fbWidth, fbHeight);
                } else {
                    command->UserCallback(commandList, command);
                }
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clipRect;
                clipRect.x = (command->ClipRect.x - clip_off.x) * clip_scale.x;
                clipRect.y = (command->ClipRect.y - clip_off.y) * clip_scale.y;
                clipRect.z = (command->ClipRect.z - clip_off.x) * clip_scale.x;
                clipRect.w = (command->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clipRect.x < static_cast<float>(fbWidth) && clipRect.y < static_cast<float>(fbHeight) &&
                    clipRect.z >= 0.0f && clipRect.w >= 0.0f) {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if (clipRect.x < 0.0f) {
                        clipRect.x = 0.0f;
                    }
                    if (clipRect.y < 0.0f) {
                        clipRect.y = 0.0f;
                    }

                    // Apply scissor/clipping rectangle
                    vk::Rect2D scissor {
                        { static_cast<int32_t>(clipRect.x), static_cast<int32_t>(clipRect.y) },
                        {
                            static_cast<uint32_t>(clipRect.z - clipRect.x),
                            static_cast<uint32_t>(clipRect.w - clipRect.y)
                        }
                    };

                    commandBuffer.setScissor(0, 1, &scissor);

                    // Draw
                    commandBuffer.drawIndexed(
                        command->ElemCount, 1, command->IdxOffset + indexOffset,
                        static_cast<int32_t>(command->VtxOffset + vertexOffset), 0
                    );
                }
            }
        }
        indexOffset += commandList->IdxBuffer.Size;
        vertexOffset += commandList->VtxBuffer.Size;
    }
}

void ImGuiSubsystem::transferVertexInformation(
    ImDrawData *drawData, vk::CommandBuffer commandBuffer, VertexAndIndexBuffer &buffers
) {
    if (drawData->TotalVtxCount <= 0) {
        return;
    }

    // Create or resize the vertex/index buffers
    vk::DeviceSize vertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    vk::DeviceSize indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);
    if (!buffers.vertex || buffers.vertex->getSize() < vertexSize) {
        buffers.vertex = engine->getBufferManager().aquire(
            vertexSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryUsage::eCPUToGPU
        );
    }
    if (!buffers.index || buffers.index->getSize() < indexSize) {
        buffers.index = engine->getBufferManager().aquire(
            indexSize, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryUsage::eCPUToGPU
        );
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    ImDrawVert *mappedVertices = nullptr;
    ImDrawIdx *mappedIndices = nullptr;
    buffers.vertex->map(reinterpret_cast<void **>(&mappedVertices));
    buffers.index->map(reinterpret_cast<void **>(&mappedIndices));

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList *commandList = drawData->CmdLists[n];
        std::memcpy(mappedVertices, commandList->VtxBuffer.Data, commandList->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(mappedIndices, commandList->IdxBuffer.Data, commandList->IdxBuffer.Size * sizeof(ImDrawIdx));
        mappedVertices += commandList->VtxBuffer.Size;
        mappedIndices += commandList->IdxBuffer.Size;
    }

    buffers.vertex->flush();
    buffers.index->flush();

    buffers.vertex->unmap();
    buffers.index->unmap();
}

void ImGuiSubsystem::prepareFrame(uint32_t activeImage) {
    ImGui::Render();
    drawData = ImGui::GetDrawData();

    // Prepare the images and descriptor sets
    uint32_t nextImagePoolIndex = 0;
    uint32_t nextTexturePoolIndex = 0;

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList *commandList = drawData->CmdLists[n];
        for (int commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; commandIndex++) {
            const ImDrawCmd *command = &commandList->CmdBuffer[commandIndex];
            Image* image;
            if (Image::isImage(command->TextureId, device)) {
                image = reinterpret_cast<Image *>(command->TextureId);
            } else {
                image = reinterpret_cast<const Texture *>(command->TextureId)->getImage().get();
            }

            auto it = imagePoolMapping.find(image);
            if (it == imagePoolMapping.end()) {
                if (nextImagePoolIndex >= MaxTexturesPerFrame) {
                    // Too many textures
                    std::cerr << "Too many textures per frame for ImGui. Only " << MaxTexturesPerFrame
                        << " are supported" << std::endl;
                    continue;
                }

                if (image->isReadyForSampling()) {
                    pipeline->updatePoolImage(0, 0, nextImagePoolIndex, *image, image->getCurrentLayout());
                } else {
                    pipeline->updatePoolImage(0, 0, nextImagePoolIndex, *image);
                }

                imagePoolMapping[image] = nextImagePoolIndex;
                ++nextImagePoolIndex;
            }
        }
    }
}

void ImGuiSubsystem::beginFrame() {
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    imagePoolMapping.clear();
}

void ImGuiSubsystem::setupFrame(
    ImDrawData *drawData, Pipeline *currentPipeline, vk::CommandBuffer commandBuffer, VertexAndIndexBuffer &buffers,
    int width, int height
) {
    // bind descriptor sets
    currentPipeline->bind(commandBuffer);

    // bind vertex and index buffer
    if (drawData->TotalVtxCount > 0) {
        vk::DeviceSize offset = 0;
        commandBuffer.bindVertexBuffers(0, 1, buffers.vertex->bufferArray(), &offset);
        commandBuffer.bindIndexBuffer(
            buffers.index->buffer(), 0, sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32
        );
    }

    // Set up viewport
    vk::Viewport viewport {
        0,
        0,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f,
        1.0f,
    };
    commandBuffer.setViewport(0, 1, &viewport);

    glm::vec2 scale { 2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y };
    ImGuiPushConstant scaleAndTranslate {
        scale,
        { -1.0f - drawData->DisplayPos.x * scale.x, -1.0f - drawData->DisplayPos.y * scale.y }
    };

    currentPipeline->push(commandBuffer, vk::ShaderStageFlagBits::eVertex, scaleAndTranslate);
}

void ImGuiSubsystem::setupFont(vk::Device device) {
    ImGuiIO &io = ImGui::GetIO();

    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    vk::DeviceSize pixelsSize = width * height * 4 * sizeof(char);

    fontImage = engine->createImage(width, height)
        .withFormat(vk::Format::eR8G8B8A8Unorm)
        .withUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
        .withDestinationStage(vk::PipelineStageFlagBits::eFragmentShader)
        .build();

    // Upload font pixels
    auto task = engine->getTaskManager().createTask();
    auto stagingBuffer = engine->getBufferManager().aquireStaging(pixelsSize);
    stagingBuffer->copyIn(pixels);

    task->execute(
        [this, &stagingBuffer](vk::CommandBuffer commandBuffer) {
            fontImage->transition(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
            fontImage->transferIn(commandBuffer, *stagingBuffer);
            fontImage->transition(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
    );

    task->freeWhenDone(std::move(stagingBuffer));

    engine->getTaskManager().submitTask(std::move(task));

    // Store our identifier
    io.Fonts->SetTexID(static_cast<ImTextureID>(*fontImage));
}

void ImGuiSubsystem::writeBarriers(vk::CommandBuffer commandBuffer) {
    for (auto &pair : imagePoolMapping) {
        if (!pair.first->isReadyForSampling()) {
            pair.first->transition(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
    }
}

bool ImGuiSubsystem::hasMouseFocus() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiSubsystem::hasKeyboardFocus() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

}
