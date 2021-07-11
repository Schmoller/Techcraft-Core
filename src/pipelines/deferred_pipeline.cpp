#include <scene/bindings.hpp>
#include "deferred_pipeline.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/device.hpp"
#include "tech-core/image.hpp"
#include "tech-core/mesh.hpp"
#include "tech-core/material/manager.hpp"
#include "tech-core/scene/entity.hpp"
#include "tech-core/scene/components/mesh_renderer.hpp"
#include "tech-core/scene/components/light.hpp"
#include "tech-core/shader/requirements.hpp"
#include "scene/components/planner_data.hpp"
#include "vulkanutils.hpp"
#include "internal/packaged/effects_screen_gen_vertex_glsl.h"
#include "internal/packaged/builtin_deferred_lighting_frag_glsl.h"
#include "internal/packaged/builtin_deferred_lighting_vert_glsl.h"
#include "internal/packaged/builtin_deferred_geom_frag_glsl.h"
#include "internal/packaged/builtin_standard_vert_glsl.h"
#include "execution_controller.hpp"

namespace Engine::Internal {

enum DeferredAttachments {
    CombinedOutput,
    Position,
    NormalRoughness,
    DiffuseOcclusion,
    Depth,
};

enum DeferredPasses {
    GeometryPass,
    LightingPass
};

enum DeferredBindings {
    CameraBinding = 0,
    EntityBinding = 1,
    LightingUniformBinding = 2,
    PositionBinding = 3,
    NormalRoughnessBinding = 4,
    DiffuseOcclusionBinding = 5,
    DepthBinding = 6,
};

DeferredPipeline::DeferredPipeline(RenderEngine &engine, VulkanDevice &device, ExecutionController &controller)
    : device(device), engine(engine), controller(controller) {
    defaultMaterial = engine.getMaterialManager().getDefault();

    geometryCommandBuffer = controller.acquireSecondaryGraphicsCommandBuffer();
    lightingCommandBuffer = controller.acquireSecondaryGraphicsCommandBuffer();
}

DeferredPipeline::~DeferredPipeline() {
    cleanupSwapChain();

    attachmentDiffuseOcclusion.reset();
    attachmentNormalRoughness.reset();
}

void DeferredPipeline::createAttachments() {
    auto attachmentBuilder = engine.createImage(framebufferSize.width, framebufferSize.height)
        .withMipLevels(1)
        .withFormat(vk::Format::eR8G8B8A8Unorm)
        .withMemoryUsage(vk::MemoryUsage::eGPUOnly)
        .withUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment)
        .withImageTiling(vk::ImageTiling::eOptimal)
        .withSampleCount(vk::SampleCountFlagBits::e1);

    attachmentDiffuseOcclusion = attachmentBuilder.build();

    auto highPBuilder = engine.createImage(framebufferSize.width, framebufferSize.height)
        .withMipLevels(1)
            // TODO: Verify that we can use this format. Find alternative if not
        .withFormat(vk::Format::eR16G16B16A16Sfloat)
        .withMemoryUsage(vk::MemoryUsage::eGPUOnly)
        .withUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment)
        .withImageTiling(vk::ImageTiling::eOptimal)
        .withSampleCount(vk::SampleCountFlagBits::e1);

    attachmentNormalRoughness = highPBuilder.build();
    attachmentPosition = highPBuilder.build();
}

void DeferredPipeline::createRenderPass() {
    std::array<vk::AttachmentDescription, 5> attachments;

    attachments[DeferredAttachments::CombinedOutput] = {
        {},
        swapChainFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    attachments[DeferredAttachments::Position] = {
        {},
        vk::Format::eR16G16B16A16Sfloat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    attachments[DeferredAttachments::NormalRoughness] = {
        {},
        vk::Format::eR16G16B16A16Sfloat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    attachments[DeferredAttachments::DiffuseOcclusion] = {
        {},
        vk::Format::eR8G8B8A8Unorm,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    attachments[DeferredAttachments::Depth] = {
        {},
        depthFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::AttachmentReference combinedOutputRef(
        DeferredAttachments::CombinedOutput, vk::ImageLayout::eColorAttachmentOptimal
    );
    vk::AttachmentReference positionOutputRef(
        DeferredAttachments::Position, vk::ImageLayout::eColorAttachmentOptimal
    );
    vk::AttachmentReference normalRoughnessOutputRef(
        DeferredAttachments::NormalRoughness, vk::ImageLayout::eColorAttachmentOptimal
    );
    vk::AttachmentReference diffuseOcclusionOutputRef(
        DeferredAttachments::DiffuseOcclusion, vk::ImageLayout::eColorAttachmentOptimal
    );
    vk::AttachmentReference depthOutputRef(
        DeferredAttachments::Depth, vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    vk::AttachmentReference positionInputRef(
        DeferredAttachments::Position, vk::ImageLayout::eShaderReadOnlyOptimal
    );
    vk::AttachmentReference normalRoughnessInputRef(
        DeferredAttachments::NormalRoughness, vk::ImageLayout::eShaderReadOnlyOptimal
    );
    vk::AttachmentReference diffuseOcclusionInputRef(
        DeferredAttachments::DiffuseOcclusion, vk::ImageLayout::eShaderReadOnlyOptimal
    );
    vk::AttachmentReference depthInputRef(
        DeferredAttachments::Depth, vk::ImageLayout::eShaderReadOnlyOptimal
    );

    std::array<vk::SubpassDescription, 2> subpasses;
    std::array<vk::SubpassDependency, 2> dependencies;

    std::array<vk::AttachmentReference, 3> geometryColorAttachments {
        positionOutputRef,
        normalRoughnessOutputRef,
        diffuseOcclusionOutputRef,
    };

    subpasses[DeferredPasses::GeometryPass] = {
        {},
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        vkUseArray(geometryColorAttachments),
        nullptr,
        &depthOutputRef
    };

    dependencies[0] = {
        VK_SUBPASS_EXTERNAL,
        DeferredPasses::GeometryPass,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlagBits::eMemoryRead,
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlagBits::eByRegion
    };

    std::array<vk::AttachmentReference, 4> lightingInputAttachments {
        positionInputRef,
        normalRoughnessInputRef,
        diffuseOcclusionInputRef,
        depthInputRef
    };
    subpasses[DeferredPasses::LightingPass] = {
        {},
        vk::PipelineBindPoint::eGraphics,
        vkUseArray(lightingInputAttachments),
        1, &combinedOutputRef,
        nullptr,
        nullptr
    };

    dependencies[1] = {
        DeferredPasses::GeometryPass,
        DeferredPasses::LightingPass,
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
            vk::PipelineStageFlagBits::eEarlyFragmentTests |
            vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits::eInputAttachmentRead,
        vk::DependencyFlagBits::eByRegion
    };

    vk::RenderPassCreateInfo renderPassInfo(
        {},
        vkUseArray(attachments),
        vkUseArray(subpasses),
        vkUseArray(dependencies)
    );

    renderPass = device.device.createRenderPass(renderPassInfo);
}

void DeferredPipeline::createFramebuffers(const Image *depthImage) {
    framebuffers.reserve(passOutputImages.size());

    for (auto &image : passOutputImages) {
        std::array<vk::ImageView, 5> mainAttachments = {
            image,
            attachmentPosition->imageView(),
            attachmentNormalRoughness->imageView(),
            attachmentDiffuseOcclusion->imageView(),
            depthImage->imageView()
        };

        vk::FramebufferCreateInfo mainFramebufferInfo(
            {},
            renderPass,
            vkUseArray(mainAttachments),
            framebufferSize.width,
            framebufferSize.height,
            1
        );

        framebuffers.emplace_back(device.device.createFramebuffer(mainFramebufferInfo));
    }
}

void DeferredPipeline::createLightingPipeline(const std::shared_ptr<Image> &depth) {
    fullScreenLightingPipeline = engine.createPipeline(renderPass, 1)
        .withInputAttachment(0, DeferredBindings::PositionBinding, attachmentPosition)
        .withInputAttachment(0, DeferredBindings::NormalRoughnessBinding, attachmentNormalRoughness)
        .withInputAttachment(0, DeferredBindings::DiffuseOcclusionBinding, attachmentDiffuseOcclusion)
        .withInputAttachment(0, DeferredBindings::DepthBinding, depth)
        .withSubpass(DeferredPasses::LightingPass)
        .withoutDepthWrite()
        .withoutDepthTest()
        .withoutFaceCulling()
        .withVertexShader(EFFECTS_SCREEN_GEN_VERTEX_GLSL, EFFECTS_SCREEN_GEN_VERTEX_GLSL_SIZE)
        .withFragmentShader(BUILTIN_DEFERRED_LIGHTING_FRAG_GLSL, BUILTIN_DEFERRED_LIGHTING_FRAG_GLSL_SIZE)
        .bindUniformBufferDynamic(1, DeferredBindings::LightingUniformBinding, vk::ShaderStageFlagBits::eFragment)
        .withColorBlend(vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eOne)
        .build();

    worldLightingPipeline = engine.createPipeline(renderPass, 1)
        .withInputAttachment(0, DeferredBindings::PositionBinding, attachmentPosition)
        .withInputAttachment(0, DeferredBindings::NormalRoughnessBinding, attachmentNormalRoughness)
        .withInputAttachment(0, DeferredBindings::DiffuseOcclusionBinding, attachmentDiffuseOcclusion)
        .withInputAttachment(0, DeferredBindings::DepthBinding, depth)
        .withSubpass(DeferredPasses::LightingPass)
        .withoutDepthWrite()
        .withVertexShader(BUILTIN_DEFERRED_LIGHTING_VERT_GLSL, BUILTIN_DEFERRED_LIGHTING_VERT_GLSL_SIZE)
        .withFragmentShader(BUILTIN_DEFERRED_LIGHTING_FRAG_GLSL, BUILTIN_DEFERRED_LIGHTING_FRAG_GLSL_SIZE)
        .bindCamera(0, DeferredBindings::CameraBinding)
        .bindUniformBufferDynamic(1, DeferredBindings::LightingUniformBinding, vk::ShaderStageFlagBits::eFragment)
        .bindUniformBufferDynamic(1, DeferredBindings::EntityBinding)
        .withVertexAttributeDescriptions(Vertex::getAttributeDescriptions())
        .withVertexBindingDescriptions(Vertex::getBindingDescription())
        .withColorBlend(vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eOne)
        .build();
}

void DeferredPipeline::createGeometryPipeline() {
    geometryPipeline = engine.createPipeline(renderPass, 3)
        .withVertexShader(BUILTIN_STANDARD_VERT_GLSL, BUILTIN_STANDARD_VERT_GLSL_SIZE)
        .withFragmentShader(BUILTIN_DEFERRED_GEOM_FRAG_GLSL, BUILTIN_DEFERRED_GEOM_FRAG_GLSL_SIZE)
        .withSubpass(DeferredPasses::GeometryPass)
        .withVertexAttributeDescriptions(Vertex::getAttributeDescriptions())
        .withVertexBindingDescriptions(Vertex::getBindingDescription())
        .bindCamera(0, Internal::StandardBindings::CameraUniform)
        .bindUniformBufferDynamic(1, Internal::StandardBindings::EntityUniform)
        .bindMaterial(2, Internal::StandardBindings::AlbedoTexture, MaterialBindPoint::Albedo)
        .bindMaterial(3, Internal::StandardBindings::NormalTexture, MaterialBindPoint::Normal)
        .build();
}

void DeferredPipeline::cleanupSwapChain() {
    for (auto framebuffer: framebuffers) {
        device.device.destroy(framebuffer);
    }

    framebuffers.clear();

    fullScreenLightingPipeline.reset();
    worldLightingPipeline.reset();
    geometryPipeline.reset();

    attachmentPosition.reset();
    attachmentNormalRoughness.reset();
    attachmentDiffuseOcclusion.reset();

    if (renderPass) {
        device.device.destroy(renderPass);
    }
}

void DeferredPipeline::recreateSwapChain(
    std::vector<vk::ImageView> outputImages, vk::Format format, vk::Extent2D size, const std::shared_ptr<Image> &depth
) {
    passOutputImages = std::move(outputImages);
    swapChainFormat = format;
    framebufferSize = size;
    depthFormat = depth->getFormat();

    createAttachments();
    createRenderPass();
    createFramebuffers(depth.get());
    createGeometryPipeline();
    createLightingPipeline(depth);
}

void DeferredPipeline::begin(uint32_t imageIndex) {
    if (framebuffers.size() > 1) {
        // Rendering to swapchain directly
        activeFramebuffer = framebuffers[imageIndex];
    } else {
        // Rendering to intermediate
        activeFramebuffer = framebuffers[0];
    }
    activeImage = imageIndex;
    lastMesh = nullptr;
    controller.beginRenderPass(renderPass, activeFramebuffer, framebufferSize, { 0, 0, 0, 0 }, 3);
}

void DeferredPipeline::beginGeometry() {
    vk::CommandBufferInheritanceInfo mainCbInheritance(
        renderPass,
        DeferredPasses::GeometryPass,
        activeFramebuffer
    );

    vk::CommandBufferBeginInfo renderBeginInfo(
        vk::CommandBufferUsageFlagBits::eRenderPassContinue,
        &mainCbInheritance
    );
    geometryCommandBuffer.begin(renderBeginInfo);

    geometryPipeline->bind(geometryCommandBuffer, activeImage);
    geometryPipeline->bindCamera(0, Internal::StandardBindings::CameraUniform, engine);
}

void DeferredPipeline::renderGeometry(const Entity *entity) {
    Engine::IsComponent auto &renderData = entity->get<MeshRenderer>();
    Engine::IsComponent auto &plannerData = entity->get<PlannerData>();
    auto mesh = renderData.getMesh();

    if (!mesh) {
        return;
    }

    if (mesh != lastMesh) {
        mesh->bind(geometryCommandBuffer);
        lastMesh = mesh;
    }

    uint32_t dyanmicOffset = plannerData.render.uniformOffset;

    std::array<vk::DescriptorSet, 1> boundDescriptors = {
        plannerData.render.buffer->set
    };

    geometryPipeline->bindDescriptorSets(geometryCommandBuffer, 1, vkUseArray(boundDescriptors), 1, &dyanmicOffset);

    auto material = renderData.getMaterial();
    if (material) {
        geometryPipeline->bindMaterial(geometryCommandBuffer, material);
    } else {
        geometryPipeline->bindMaterial(geometryCommandBuffer, defaultMaterial);
    }

    geometryCommandBuffer.drawIndexed(mesh->getIndexCount(), 1, 0, 0, 0);
}

void DeferredPipeline::endGeometry() {
    geometryCommandBuffer.end();
    controller.addToRender(geometryCommandBuffer);
}

void DeferredPipeline::beginLighting() {
    vk::CommandBufferInheritanceInfo mainCbInheritance(
        renderPass,
        DeferredPasses::LightingPass,
        activeFramebuffer
    );

    vk::CommandBufferBeginInfo renderBeginInfo(
        vk::CommandBufferUsageFlagBits::eRenderPassContinue,
        &mainCbInheritance
    );
    lightingCommandBuffer.begin(renderBeginInfo);

    controller.nextSubpass();

    fullScreenLights.clear();
    worldLights.clear();
}

void DeferredPipeline::renderLight(const Entity *entity) {
    Engine::IsComponent auto &light = entity->get<Light>();

    // TODO: Use the fullscreen one only for lights which cross the near field or directional lights
    // Use the 3D one for all other lights
//    if (light.getType() == LightType::Directional) {
    fullScreenLights.emplace_back(entity);
//    } else {
//        worldLights.emplace_back(entity);
//    }
}

void DeferredPipeline::endLighting() {
    // FIXME: Need to render one anyway otherwise we get blank

    fullScreenLightingPipeline->bind(lightingCommandBuffer, activeImage);
    for (auto entity : fullScreenLights) {
        Engine::IsComponent auto &plannerData = entity->get<PlannerData>();
        uint32_t dynamicOffset = plannerData.light.uniformOffset;

        std::array<vk::DescriptorSet, 1> boundDescriptors = {
            plannerData.light.buffer->set
        };

        fullScreenLightingPipeline->bindDescriptorSets(
            lightingCommandBuffer, 1, vkUseArray(boundDescriptors), 1, &dynamicOffset
        );
        lightingCommandBuffer.draw(3, 1, 0, 0);
    }
    // TODO: Render all lights woo

    // Temporary
//    fullScreenLightingPipeline->bind(lightingCommandBuffer, activeImage);
//    lightingCommandBuffer.draw(3, 1, 0, 0);

    lightingCommandBuffer.end();
    controller.addToRender(lightingCommandBuffer);
}

void DeferredPipeline::end() {
    controller.endRenderPass();
}

PipelineRequirements DeferredPipeline::getRequirements() const {
    PipelineRequirements requirements;
    requirements.addOutputAttachment(
        {
            0,
            ShaderValueType::Vec4,
        }
    );
    requirements.addOutputAttachment(
        {
            1,
            ShaderValueType::Vec4,
        }
    );
    requirements.addOutputAttachment(
        {
            2,
            ShaderValueType::Vec4,
        }
    );

    requirements.addVertexDefinition(
        {
            0,
            ShaderValueType::Vec3
        }
    );
    requirements.addVertexDefinition(
        {
            1,
            ShaderValueType::Vec3
        }
    );
    requirements.addVertexDefinition(
        {
            2,
            ShaderValueType::Vec3
        }
    );
    requirements.addVertexDefinition(
        {
            3,
            ShaderValueType::Vec4
        }
    );
    requirements.addVertexDefinition(
        {
            4,
            ShaderValueType::Vec2
        }
    );

    return requirements;
}
}