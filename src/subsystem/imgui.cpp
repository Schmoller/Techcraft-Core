#include "tech-core/subsystem/imgui.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/task.hpp"
#include "tech-core/pipeline.hpp"
#include "vulkanutils.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <glm/glm.hpp>

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

    // create font sampler
    fontSampler = device.createSampler(
        vk::SamplerCreateInfo(
            {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear
        )
            .setMinLod(-1000)
            .setMaxLod(1000)
            .setMaxAnisotropy(1.0f)
    );

    std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {{
        { // font sampler
            0,
            vk::DescriptorType::eCombinedImageSampler,
            1, // count
            vk::ShaderStageFlagBits::eFragment,
            &fontSampler
        }
    }};

    imageSamplerLayout = device.createDescriptorSetLayout(
        {
            {}, vkUseArray(bindings)
        }
    );

    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {
            vk::DescriptorType::eCombinedImageSampler,
            1
        }
    }};


    descriptorPool = device.createDescriptorPool(
        {
            {},
            1,
            vkUseArray(poolSizes)
        }
    );


    // Descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(1, imageSamplerLayout);

    imageSamplerDS = device.allocateDescriptorSets(
        {
            descriptorPool,
            vkUseArray(layouts)
        }
    );

    setupFont(device);
}

void ImGuiSubsystem::initialiseSwapChainResources(
    vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages
) {
    pipeline = engine.createPipeline()
        .withVertexShader("assets/shaders/imgui-vert.spv")
        .withFragmentShader("assets/shaders/imgui-frag.spv")
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
        .withDescriptorSet(imageSamplerLayout)
        .withPushConstants<ImGuiPushConstant>(vk::ShaderStageFlagBits::eVertex)
        .withoutDepthWrite()
        .withoutDepthTest()
        .build();

    vertexBuffers.resize(swapChainImages);
}

void ImGuiSubsystem::cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) {
    pipeline.reset();
}

void ImGuiSubsystem::cleanupResources(vk::Device device, _E::RenderEngine &engine) {
    fontImage.reset();
    device.destroy(fontSampler);
    device.destroy(imageSamplerLayout);
    device.destroy(descriptorPool);

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
    setupFrame(drawData, commandBuffer, buffers, fbWidth, fbHeight);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int vertexOffset = 0;
    int indexOffset = 0;
    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList *commandList = drawData->CmdLists[n];
        for (int commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; commandIndex++) {
            const ImDrawCmd *command = &commandList->CmdBuffer[commandIndex];
            if (command->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (command->UserCallback == ImDrawCallback_ResetRenderState) {
                    setupFrame(drawData, commandBuffer, buffers, fbWidth, fbHeight);
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
}

void ImGuiSubsystem::beginFrame() {
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiSubsystem::setupFrame(
    ImDrawData *drawData, vk::CommandBuffer commandBuffer, VertexAndIndexBuffer &buffers, int width, int height
) {
    // bind descriptor sets
    pipeline->bind(commandBuffer);
    pipeline->bindDescriptorSets(commandBuffer, 0, 1, imageSamplerDS.data(), 0, nullptr);

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

    pipeline->push(commandBuffer, vk::ShaderStageFlagBits::eVertex, scaleAndTranslate);

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

    // Update the DS
    vk::DescriptorImageInfo imageInfo(fontSampler, fontImage->imageView(), vk::ImageLayout::eShaderReadOnlyOptimal);

    std::array<vk::WriteDescriptorSet, 1> descriptorUpdate {
        vk::WriteDescriptorSet(imageSamplerDS[0])
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&imageInfo)
    };

    device.updateDescriptorSets(descriptorUpdate, {});

    // Upload font pixels
    auto task = engine->getTaskManager().createTask();

    task->execute(
        [this, pixels, pixelsSize](vk::CommandBuffer commandBuffer) {
            fontImage->transition(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
            fontImage->transfer(commandBuffer, pixels, pixelsSize);
            fontImage->transition(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
    );

    task->executeWhenComplete(
        [this]() {
            fontImage->completeTransfer();
        }
    );

    engine->getTaskManager().submitTask(std::move(task));

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID) reinterpret_cast<intptr_t>(static_cast<VkImage>(fontImage->image())));
}

bool ImGuiSubsystem::hasMouseFocus() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiSubsystem::hasKeyboardFocus() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

}