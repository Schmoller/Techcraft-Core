#include "tech-core/gui/manager.hpp"
#include "tech-core/gui/drawer.hpp"
#include "tech-core/pipeline.hpp"
#include "tech-core/texture/manager.hpp"
#include <iostream>

namespace Engine::Gui {

GuiManager::GuiManager(
    vk::Device device,
    Engine::TextureManager &textureManager,
    Engine::BufferManager &bufferManager,
    Engine::TaskManager &taskManager,
    Engine::FontManager &fontManager,
    Engine::PipelineBuilder pipelineBuilder,
    const vk::Extent2D &windowSize
) : device(device),
    textureManager(textureManager),
    bufferManager(bufferManager),
    taskManager(taskManager),
    fontManager(fontManager),
    windowSize(windowSize) {
    recreatePipeline(pipelineBuilder, windowSize);

    combinedVertexIndexBuffer = bufferManager.aquireDivisible(
        MAX_GUI_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryUsage::eCPUToGPU
    );

    vk::SamplerCreateInfo samplerInfo = {
        {},
        vk::Filter::eNearest,
        vk::Filter::eNearest,
        vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge
    };
    sampler = device.createSamplerUnique(samplerInfo);
}

void GuiManager::recreatePipeline(Engine::PipelineBuilder pipelineBuilder, const vk::Extent2D &windowSize) {
    pipeline = pipelineBuilder
        .withVertexShader("assets/shaders/gui-vert.spv")
        .withFragmentShader("assets/shaders/gui-frag.spv")
        .withoutDepthTest()
        .withoutDepthWrite()
        .bindTextures(0, 2)
        .withPushConstants<GuiPC>(vk::ShaderStageFlagBits::eVertex)
        .withVertexBindingDescription(Vertex::getBindingDescription())
        .withVertexAttributeDescriptions(Vertex::getAttributeDescriptions())
        .withoutFaceCulling()
        .withAlpha()
        .build();

    this->windowSize = windowSize;

    viewState.view = glm::orthoRH_ZO(
        0.0f,
        static_cast<float>(windowSize.width),
        0.0f,
        static_cast<float>(windowSize.height),
        10000000.0f,
        0.0f
    );

    for (auto &component : components) {
        component.second.component->onScreenResize(windowSize.width, windowSize.height);
    }
}

uint16_t GuiManager::addComponent(std::shared_ptr<BaseComponent> component) {
    auto id = nextId++;
    auto pair = components.emplace(
        id, ComponentMapping {
            id,
            component,
        }
    );

    auto &mapping = pair.first->second;

    renderComponent(component.get(), mapping);
    component->onRegister(
        mapping.id,
        [this, &mapping]() { markComponentDirty(mapping.id); }
    );

    return mapping.id;
}

void GuiManager::removeComponent(uint16_t id) {
    auto it = components.find(id);
    if (it == components.end()) {
        return;
    }

    // Free used sections
    auto mapping = it->second;
    for (auto &region : mapping.regions) {
        combinedVertexIndexBuffer->freeSection(region.offset, region.size);
    }

    components.erase(it);
}

void GuiManager::updateComponent(uint16_t id) {
    // TODO: Regenerate the vertices and indices for this component

    if (components.count(id)) {
        auto &component = components[id];

        renderComponent(component.component.get(), component);
    }
}

void GuiManager::renderComponent(BaseComponent *component, ComponentMapping &mapping) {
    // Renders the vertices and indices
    Drawer drawer(fontManager, *textureManager.get("internal.white"), "monospace");

    component->onRender(drawer);

    // Remove all previously existing regions
    for (auto &region : mapping.regions) {
        combinedVertexIndexBuffer->freeSection(region.offset, region.size);
    }

    mapping.regions.clear();

    if (drawer.regions.empty()) {
        return;
    }

    mapping.regions.reserve(drawer.regions.size());

    // Allocate all new regions
    for (auto &region : drawer.regions) {
        // Allocate space
        vk::DeviceSize vertexSize = sizeof(Vertex) * region.vertices.size();
        vk::DeviceSize indexSize = sizeof(GuiBufferInt) * region.indices.size();

        vk::DeviceSize offset;
        vk::DeviceSize indexOffset;
        vk::DeviceSize size = vertexSize + indexSize;

        // New buffer region
        offset = combinedVertexIndexBuffer->allocateSection(size);
        if (offset == ALLOCATION_FAILED) {
            throw std::runtime_error("Out of GUI buffer memory");
        }

        indexOffset = offset + vertexSize;

        // Load up the buffer
        combinedVertexIndexBuffer->copyIn(region.vertices.data(), offset, vertexSize);
        combinedVertexIndexBuffer->copyIn(region.indices.data(), indexOffset, indexSize);
        combinedVertexIndexBuffer->flushRange(offset, vertexSize + indexSize);

        mapping.regions.emplace_back(
            ComponentMapping::Region {
                region.texture,
                offset,
                size,
                region.vertices.size(),
                region.indices.size(),
                indexOffset
            }
        );
    }

    // std::cout << "Rendered component " << mapping.vertStart << "(" << mapping.vertCount << ") - " << mapping.indexStart << "(" << mapping.indexCount << ")";
    // if (mapping.texture) {
    //     std::cout << " texture: " << mapping.texture->arrayId << "[" << mapping.texture->arraySlot << "]";
    // } else {
    //     std::cout << " texture: none";
    // }

    // std::cout << std::endl;
}

void GuiManager::markComponentDirty(uint16_t id) {
    dirtyComponents.push_back(id);
}

void GuiManager::update() {
    // Update all components
    Rect windowBounds = {{ 0, 0 }, { windowSize.width, windowSize.height }};

    for (auto &pair : components) {
        auto &component = pair.second.component;

        if (component->needsLayout()) {
            component->onLayout(windowBounds);
        }

        component->onFrameUpdate();
    }

    // Refresh all dirty components
    for (auto id : dirtyComponents) {
        updateComponent(id);
    }

    dirtyComponents.clear();
}

void GuiManager::render(vk::CommandBuffer commandBuffer, vk::CommandBufferInheritanceInfo &cbInheritance) {
    // TODO: In the future consider using indirect rendering to draw the UI
    //https://github.com/SaschaWillems/Vulkan/tree/master/examples/indirectdraw

    vk::CommandBufferBeginInfo beginInfo(
        vk::CommandBufferUsageFlagBits::eRenderPassContinue,
        &cbInheritance
    );
    commandBuffer.begin(beginInfo);

    pipeline->bind(commandBuffer);
    pipeline->push(commandBuffer, vk::ShaderStageFlagBits::eVertex, viewState);
    bool hasBoundTexture = false;
//
//    // Wait for any changes to be propagated before rendering
//    // Prevents weird artifacts with partially transferred data being rendered
//    vk::MemoryBarrier barrier {
//        vk::AccessFlagBits::eTransferWrite,
//        vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eIndexRead
//    };
//
//    commandBuffer.pipelineBarrier(
//        vk::PipelineStageFlagBits::eTransfer,
//        vk::PipelineStageFlagBits::eVertexInput,
//        vk::DependencyFlagBits::eDeviceGroup,
//        1, &barrier,
//        0, nullptr,
//        0, nullptr
//    );

    for (auto &pair : components) {
        auto &component = pair.second;

        for (auto &region : component.regions) {
            // Vertex info
            commandBuffer.bindVertexBuffers(0, 1, combinedVertexIndexBuffer->bufferArray(), &region.offset);
            commandBuffer.bindIndexBuffer(
                combinedVertexIndexBuffer->buffer(), region.indexOffset, vk::IndexType::eUint16
            );

            // texture info
            if (region.texture) {
                pipeline->bindTexture(commandBuffer, 0, region.texture);
                hasBoundTexture = true;
            } else if (!hasBoundTexture) {
                // It is required that all bindings have a value, even if we wont use one
                pipeline->bindTexture(commandBuffer, 0, textureManager.get("internal.white"));
                hasBoundTexture = true;
            }

            commandBuffer.drawIndexed(region.indexCount, 1, 0, 0, 0);
        }
    }

    commandBuffer.end();
}

void GuiManager::processAnchors(BaseComponent *component) {
    auto &anchor = component->getAnchor();
    auto &offsets = component->getAnchorOffsets();

    auto bounds = component->getBounds();
    bool changed = false;

    if (anchor & AnchorFlag::Center) {
        float newX = (windowSize.width - bounds.width()) / 2;
        if (bounds.topLeft.x != newX) {
            bounds.bottomRight.x = newX + bounds.width();
            bounds.topLeft.x = newX;
            changed = true;
        }

        float newY = (windowSize.height - bounds.height()) / 2;
        if (bounds.topLeft.y != newY) {
            bounds.bottomRight.y = newY + bounds.height();
            bounds.topLeft.y = newY;
            changed = true;
        }
    }

    if (anchor & AnchorFlag::Top) {
        float newY = offsets.top;
        if (bounds.topLeft.y != newY) {
            bounds.topLeft.y = newY;
            changed = true;
        }
    }
    if (anchor & AnchorFlag::Left) {
        float newX = offsets.left;
        if (bounds.topLeft.x != newX) {
            bounds.topLeft.x = newX;
            changed = true;
        }
    }
    if (anchor & AnchorFlag::Bottom) {
        float newY = windowSize.height - offsets.bottom;
        if (bounds.bottomRight.y != newY) {
            bounds.bottomRight.y = newY;
            changed = true;
        }
    }
    if (anchor & AnchorFlag::Right) {
        float newX = windowSize.width - offsets.right;
        if (bounds.bottomRight.x != newX) {
            bounds.bottomRight.x = newX;
            changed = true;
        }
    }

    if (changed) {
        component->setBounds(bounds);
    }
}
}