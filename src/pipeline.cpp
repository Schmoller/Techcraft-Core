#include "tech-core/pipeline.hpp"
#include "vulkanutils.hpp"
#include "tech-core/mesh.hpp"
#include "tech-core/image.hpp"
#include "tech-core/buffer.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/material/material.hpp"
#include "texture/descriptor_cache.hpp"


#include <array>
#include <exception>
#include <utility>

namespace Engine {

PipelineBuilder::PipelineBuilder(
    RenderEngine &engine,
    vk::Device device,
    vk::RenderPass renderPass,
    uint32_t colorAttachmentCount,
    vk::Extent2D windowSize,
    uint32_t swapChainImages,
    Internal::DescriptorCacheManager &descriptorManager
) : engine(engine),
    device(device),
    renderPass(renderPass),
    colorAttachmentCount(colorAttachmentCount),
    windowSize(windowSize),
    swapChainImages(swapChainImages),
    depthTestEnable(true),
    depthWriteEnable(true),
    cullFaces(true),
    alpha(false),
    descriptorManager(descriptorManager) {}

PipelineBuilder &PipelineBuilder::withVertexShader(const std::string &path) {
    vertexShaderData = readFile(path);

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexShader(const char *data, size_t size) {
    vertexShaderData.resize(size);
    std::memcpy(vertexShaderData.data(), data, size);

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexShader(const unsigned char *data, size_t size) {
    vertexShaderData.resize(size);
    std::memcpy(vertexShaderData.data(), data, size);

    return *this;
}

PipelineBuilder &PipelineBuilder::withFragmentShader(const std::string &path) {
    fragmentShaderData = readFile(path);

    return *this;
}

PipelineBuilder &PipelineBuilder::withFragmentShader(const char *data, size_t size) {
    fragmentShaderData.resize(size);
    std::memcpy(fragmentShaderData.data(), data, size);

    return *this;
}

PipelineBuilder &PipelineBuilder::withFragmentShader(const unsigned char *data, size_t size) {
    fragmentShaderData.resize(size);
    std::memcpy(fragmentShaderData.data(), data, size);

    return *this;
}

PipelineBuilder &PipelineBuilder::withShaderConstant(uint32_t constantId, vk::ShaderStageFlagBits stage, bool value) {
    if (stage == vk::ShaderStageFlagBits::eVertex) {
        uint32_t offset = sizeof(uint32_t) * vertexSpecializationData.size();
        vertexSpecializationData.push_back(static_cast<VkBool32>(value));
        vertexSpecializationEntries.emplace_back(constantId, offset, sizeof(VkBool32));
    } else if (stage == vk::ShaderStageFlagBits::eFragment) {
        uint32_t offset = sizeof(uint32_t) * fragmentSpecializationData.size();
        fragmentSpecializationData.push_back(static_cast<VkBool32>(value));
        fragmentSpecializationEntries.emplace_back(constantId, offset, sizeof(VkBool32));
    } else {
        throw std::runtime_error("Invalid stage");
    }

    return *this;
}

PipelineBuilder &PipelineBuilder::withGeometryType(PipelineGeometryType type) {
    geomType = type;
    return *this;
}

PipelineBuilder &PipelineBuilder::withDescriptorSet(vk::DescriptorSetLayout ds) {
    providedDescriptorLayouts.push_back(ds);

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexBindingDescription(const vk::VertexInputBindingDescription &binding) {
    vertexBindings.push_back(binding);

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexBindingDescriptions(
    const vk::ArrayProxy<const vk::VertexInputBindingDescription> &bindings
) {
    for (auto binding : bindings) {
        vertexBindings.push_back(binding);
    }

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexAttributeDescription(const vk::VertexInputAttributeDescription &attribute) {
    vertexAttributes.push_back(attribute);

    return *this;
}

PipelineBuilder &PipelineBuilder::withVertexAttributeDescriptions(
    const vk::ArrayProxy<const vk::VertexInputAttributeDescription> &attributes
) {
    for (auto attribute : attributes) {
        vertexAttributes.push_back(attribute);
    }

    return *this;
}

PipelineBuilder &PipelineBuilder::withoutFaceCulling() {
    cullFaces = false;
    return *this;
}

PipelineBuilder &PipelineBuilder::withoutDepthWrite() {
    depthWriteEnable = false;

    return *this;
}

PipelineBuilder &PipelineBuilder::withoutDepthTest() {
    depthTestEnable = false;

    return *this;
}

PipelineBuilder &PipelineBuilder::withAlpha() {
    alpha = true;

    return *this;
}

PipelineBuilder &PipelineBuilder::withFillMode(FillMode mode) {
    fillMode = mode;
    return *this;
}

PipelineBuilder &PipelineBuilder::withDynamicState(vk::DynamicState state) {
    dynamicState.push_back(state);
    return *this;
}

PipelineBuilder &PipelineBuilder::withSubpass(uint32_t subpassIndex) {
    subpass = subpassIndex;
    return *this;
}

PipelineBuilder &PipelineBuilder::bindCamera(uint32_t set, uint32_t binding) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::PerSwapChain,
            {
                binding,
                vk::DescriptorType::eUniformBuffer,
                1,
                vk::ShaderStageFlagBits::eVertex
            },
            SpecialBinding::Camera
        }
    );

    return *this;
}

PipelineBuilder &PipelineBuilder::bindTextures(uint32_t set, uint32_t binding) {
    for (auto &existing : bindings) {
        if (existing.type == SpecialBinding::Textures && existing.set == set) {
            throw std::runtime_error("Cannot bind multiple textures to same set");
        }
    }

    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::External,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                vk::ShaderStageFlagBits::eFragment
            },
            SpecialBinding::Textures
        }
    );

    return *this;
}

PipelineBuilder &PipelineBuilder::bindMaterial(uint32_t set, uint32_t binding, MaterialBindPoint bindPoint) {
    bindTextures(set, binding);

    materialBindings[bindPoint] = binding;
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImage(
    uint32_t set, uint32_t binding, const vk::ShaderStageFlags &stages, vk::Sampler sampler
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImage(
    uint32_t set, uint32_t binding, std::shared_ptr<Image> image, const vk::ShaderStageFlags &stages,
    vk::Sampler sampler
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            std::move(image)
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImage(
    uint32_t set, uint32_t binding, const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout, vk::Sampler sampler
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            {},
            imageLayout
        }
    );
    return *this;
}


PipelineBuilder &PipelineBuilder::bindSampledImage(
    uint32_t set, uint32_t binding, std::shared_ptr<Image> image, const vk::ShaderStageFlags &stages,
    vk::ImageLayout imageLayout, vk::Sampler sampler
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            std::move(image),
            imageLayout
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImageImmutable(
    uint32_t set, uint32_t binding, vk::Sampler sampler, const vk::ShaderStageFlags &stages
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            {},
            vk::ImageLayout::eShaderReadOnlyOptimal,
            {},
            true
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImageImmutable(
    uint32_t set, uint32_t binding, vk::Sampler sampler, const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            {},
            imageLayout,
            {},
            true
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImageImmutable(
    uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::Sampler sampler,
    const vk::ShaderStageFlags &stages
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            std::move(image),
            vk::ImageLayout::eShaderReadOnlyOptimal,
            {},
            true
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImageImmutable(
    uint32_t set, uint32_t binding, std::shared_ptr<Image> image, vk::Sampler sampler,
    const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            std::move(image),
            imageLayout,
            {},
            true
        }
    );
    return *this;
}

PipelineBuilder &
PipelineBuilder::bindUniformBuffer(uint32_t set, uint32_t binding, const vk::ShaderStageFlags &stages) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eUniformBuffer,
                1,
                stages
            }
        }
    );
    return *this;
}


PipelineBuilder &PipelineBuilder::bindUniformBuffer(
    uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer, const vk::ShaderStageFlags &stages
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eUniformBuffer,
                1,
                stages
            },
            SpecialBinding::None,
            {},
            {},
            vk::ImageLayout::eUndefined,
            std::move(buffer)
        }
    );

    return *this;
}

PipelineBuilder &
PipelineBuilder::bindUniformBufferDynamic(uint32_t set, uint32_t binding, const vk::ShaderStageFlags &stages) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eUniformBufferDynamic,
                1,
                stages
            }
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindUniformBufferDynamic(
    uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer, const vk::ShaderStageFlags &stages
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eUniformBufferDynamic,
                1,
                stages
            },
            SpecialBinding::None,
            {},
            {},
            vk::ImageLayout::eUndefined,
            std::move(buffer)
        }
    );

    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImagePool(
    uint32_t set, uint32_t binding, uint32_t size, const vk::ShaderStageFlags &stages, vk::Sampler sampler
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Pool,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            {},
            vk::ImageLayout::eShaderReadOnlyOptimal,
            {},
            false,
            size
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImagePool(
    uint32_t set, uint32_t binding, uint32_t size, const vk::ShaderStageFlags &stages, vk::ImageLayout imageLayout,
    vk::Sampler sampler
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Pool,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            {},
            imageLayout,
            {},
            false,
            size
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImagePoolImmutable(
    uint32_t set, uint32_t binding, uint32_t size, vk::Sampler sampler, const vk::ShaderStageFlags &stages
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Pool,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            {},
            vk::ImageLayout::eShaderReadOnlyOptimal,
            {},
            true,
            size
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::bindSampledImagePoolImmutable(
    uint32_t set, uint32_t binding, uint32_t size, vk::Sampler sampler, const vk::ShaderStageFlags &stages,
    vk::ImageLayout imageLayout
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Pool,
            {
                binding,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                stages
            },
            SpecialBinding::None,
            sampler,
            {},
            imageLayout,
            {},
            true,
            size
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::withInputAttachment(
    uint32_t set, uint32_t binding, const vk::ShaderStageFlags &stages
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eInputAttachment,
                1,
                stages
            },
            SpecialBinding::None,
            {},
            {},
            vk::ImageLayout::eShaderReadOnlyOptimal,
        }
    );
    return *this;
}

PipelineBuilder &PipelineBuilder::withInputAttachment(
    uint32_t set, uint32_t binding, std::shared_ptr<Image> image, const vk::ShaderStageFlags &stages
) {
    bindings.emplace_back(
        PipelineBinding {
            set,
            binding,
            BindingCount::Single,
            {
                binding,
                vk::DescriptorType::eInputAttachment,
                1,
                stages
            },
            SpecialBinding::None,
            {},
            std::move(image),
            vk::ImageLayout::eShaderReadOnlyOptimal,
        }
    );
    return *this;
}

void PipelineBuilder::reconfigure(vk::RenderPass renderPass, vk::Extent2D windowSize) {
    this->renderPass = renderPass;
    this->windowSize = windowSize;
}

void PipelineBuilder::processBindings(
    std::vector<vk::DescriptorSetLayout> &layouts, std::vector<uint32_t> &setCounts,
    std::vector<vk::DescriptorPoolSize> &poolSizes, uint32_t &totalSets, std::vector<bool> &autoBindSet
) {
    std::map<vk::DescriptorType, uint32_t> counters;
    std::multimap<u_int32_t, vk::DescriptorSetLayoutBinding> bindingsBySet;
    uint32_t maxSet = 0;

    autoBindSet.resize(0);
    setCounts.resize(0);

    for (auto &binding : bindings) {
        if (binding.isSamplerImmutable) {
            binding.definition.setPImmutableSamplers(&binding.sampler);
        }
        bindingsBySet.emplace(binding.set, binding.definition);
        maxSet = std::max(maxSet, binding.set);

        // Work out how many descriptor types are required for the pool
        auto type = binding.definition.descriptorType;
        auto it = counters.find(type);
        uint32_t count;
        if (binding.count == BindingCount::Single) {
            count = 1;
        } else if (binding.count == BindingCount::External) {
            count = 0;
        } else if (binding.count == BindingCount::Pool) {
            count = binding.poolSize;
        } else {
            count = swapChainImages;
        }

        if (setCounts.size() <= maxSet) {
            setCounts.resize(maxSet + 1);
        }
        setCounts[binding.set] = std::max(setCounts[binding.set], count);

        if (it == counters.end()) {
            if (count > 0) {
                counters[type] = count;
            }
        } else {
            it->second += count;
        }

        if (autoBindSet.size() <= maxSet) {
            uint32_t start = autoBindSet.size();
            autoBindSet.resize(maxSet + 1);
            std::fill(autoBindSet.begin() + start, autoBindSet.end(), true);
        }

        bool autoBind;
        switch (binding.definition.descriptorType) {
            case vk::DescriptorType::eUniformBufferDynamic:
                autoBind = false;
                break;
            default:
                autoBind = true;
                break;
        }

        autoBindSet[binding.set] = autoBindSet[binding.set] && autoBind;
    }

    totalSets = 0;
    for (auto set = 0; set <= maxSet; ++set) {
        auto range = bindingsBySet.equal_range(set);
        std::vector<vk::DescriptorSetLayoutBinding> setBindings;
        for (auto it = range.first; it != range.second; ++it) {
            setBindings.push_back(it->second);
        }

        if (setBindings.empty()) {
            continue;
        }

        layouts.push_back(
            device.createDescriptorSetLayout(
                {
                    {},
                    vkUseArray(setBindings)
                }
            )
        );

        totalSets += setCounts[set];
    }

    // Setup pool settings
    poolSizes.resize(counters.size());
    size_t index = 0;
    for (auto &pair : counters) {
        poolSizes[index++] = {
            pair.first,
            pair.second
        };
    }
}

std::unique_ptr<Pipeline> PipelineBuilder::build() {
    // Set up the shader stages
    vk::ShaderModule vertShaderModule = createShaderModule(device, vertexShaderData);
    vk::ShaderModule fragShaderModule = createShaderModule(device, fragmentShaderData);

    vk::SpecializationInfo vertShaderSpecialization(
        vkUseArray(vertexSpecializationEntries), sizeof(uint32_t) * vertexSpecializationData.size(),
        vertexSpecializationData.data()
    );

    vk::SpecializationInfo fragShaderSpecialization(
        vkUseArray(fragmentSpecializationEntries), sizeof(uint32_t) * fragmentSpecializationData.size(),
        fragmentSpecializationData.data()
    );

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main", &vertShaderSpecialization
    );

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main", &fragShaderSpecialization
    );

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

    // Descriptor layouts
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<uint32_t> setCounts;
    std::vector<vk::DescriptorPoolSize> poolSizes;
    std::vector<bool> autoBindSet;
    uint32_t totalSets;

    processBindings(descriptorSetLayouts, setCounts, poolSizes, totalSets, autoBindSet);

    vk::DescriptorPool descriptorPool;
    if (totalSets != 0) {
        // Create the descriptor sets
        descriptorPool = device.createDescriptorPool(
            {
                {},
                totalSets,
                vkUseArray(poolSizes)
            }
        );
    }

    std::vector<std::vector<vk::DescriptorSet>> descriptorSets;
    for (auto set = 0; set < descriptorSetLayouts.size(); ++set) {
        if (setCounts[set] > 0) {
            std::vector<vk::DescriptorSetLayout> forAllocation(setCounts[set], descriptorSetLayouts[set]);

            auto allocatedSets = device.allocateDescriptorSets(
                {
                    descriptorPool,
                    vkUseArray(forAllocation)
                }
            );

            descriptorSets.push_back(allocatedSets);
        } else {
            descriptorSets.emplace_back();
        }
    }

    // DEBUG FIXME: This is just temporary to keep interop
    std::vector<vk::DescriptorSetLayout> ownedLayouts = descriptorSetLayouts;
    for (auto &set : providedDescriptorLayouts) {
        descriptorSetLayouts.push_back(set);
    }

    // Vertex format
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
        {},
        vkUseArray(vertexBindings),
        vkUseArray(vertexAttributes)
    );

    // Index format
    vk::PrimitiveTopology topology;
    switch (geomType) {
        case PipelineGeometryType::Polygons:
            topology = vk::PrimitiveTopology::eTriangleList;
            break;
        case PipelineGeometryType::SegmentedLines:
            topology = vk::PrimitiveTopology::eLineList;
            break;
        case PipelineGeometryType::ContinousLines:
            topology = vk::PrimitiveTopology::eLineStrip;
            break;
    }

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
        {},
        topology,
        VK_FALSE
    );

    // Viewport and scissor
    vk::Viewport viewport(
        0.0f,
        0.0f,
        (float) windowSize.width,
        (float) windowSize.height,
        0.0f,
        1.0f
    );

    vk::Rect2D scissor({ 0, 0 }, windowSize);

    vk::PipelineViewportStateCreateInfo viewportState(
        {},
        1, &viewport,
        1, &scissor
    );

    // Culling
    vk::CullModeFlags cullMode;
    if (cullFaces) {
        cullMode = vk::CullModeFlagBits::eBack;
    } else {
        cullMode = vk::CullModeFlagBits::eNone;
    }

    vk::PolygonMode polygonMode;
    switch (fillMode) {
        case FillMode::Solid:
            polygonMode = vk::PolygonMode::eFill;
            break;
        case FillMode::Wireframe:
            polygonMode = vk::PolygonMode::eLine;
            break;
        case FillMode::Point:
            polygonMode = vk::PolygonMode::ePoint;
            break;
    }

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {},
        VK_FALSE,
        VK_FALSE,
        polygonMode,
        cullMode,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0,
        0,
        0,
        1.0f
    );

    // Other non-configurable
    vk::PipelineMultisampleStateCreateInfo multisampling(
        {},
        vk::SampleCountFlagBits::e1,
        VK_FALSE,
        1.0f
    );

    // Blending
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(colorAttachmentCount);

    for (auto i = 0; i < colorAttachmentCount; ++i) {
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA
        );

        if (alpha) {
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
            colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
            colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
            colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        }

        colorBlendAttachments[i] = colorBlendAttachment;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {},
        VK_FALSE,
        vk::LogicOp::eCopy,
        vkUseArray(colorBlendAttachments)
    );
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},
        vkUseArray(descriptorSetLayouts),
        vkUseArray(pushConstants)
    );

    auto pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    vk::PipelineDepthStencilStateCreateInfo depthStencil;

    depthStencil = vk::PipelineDepthStencilStateCreateInfo(
        {},
        depthTestEnable,
        depthWriteEnable,
        vk::CompareOp::eLess,
        VK_FALSE,
        VK_FALSE,
        {},
        {},
        0.0f,
        1.0f
    );

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo(
        {}, vkUseArray(dynamicState)
    );

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        vkUseArray(shaderStages),
        &vertexInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        &depthStencil,
        &colorBlending,
        &dynamicStateInfo,
        pipelineLayout,
        renderPass,
        subpass,
        vk::Pipeline(),
        -1
    );

    auto vulkanPipeline = device.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);

    device.destroyShaderModule(vertShaderModule);
    device.destroyShaderModule(fragShaderModule);

    std::map<uint32_t, Pipeline::PipelineBindingDetails> pipelineBindingDetails;
    std::map<uint32_t, std::shared_ptr<Internal::DescriptorCache>> textureDescriptorCaches;
    for (auto &binding : bindings) {
        pipelineBindingDetails[binding.binding] = {
            binding.set,
            binding.definition.descriptorType,
            binding.targetLayout,
            binding.sampler
        };

        if (binding.type == SpecialBinding::Textures) {
            auto descriptorCache = descriptorManager.get(binding.binding);
            textureDescriptorCaches.emplace(binding.binding, descriptorCache);
        }
    }

    auto pipeline = std::unique_ptr<Pipeline>(
        new Pipeline(
            device, {
                vulkanPipeline,
                pipelineLayout,
                descriptorSets,
                autoBindSet,
                ownedLayouts,
                descriptorPool
            },
            pipelineBindingDetails,
            textureDescriptorCaches,
            materialBindings
        )
    );

    // Bind any resources already provided
    for (auto &binding : bindings) {
        auto image = binding.image.lock();
        auto buffer = binding.buffer.lock();
        if (image) {
            pipeline->bindImage(binding.set, binding.binding, image, binding.sampler);
        } else if (buffer) {
            pipeline->bindBuffer(binding.set, binding.binding, buffer);
        } else if (binding.type == SpecialBinding::Camera) {
            pipeline->bindCamera(binding.set, binding.binding, engine);
        }
    }

    return pipeline;
}

Pipeline::Pipeline(
    vk::Device device, PipelineResources resources, std::map<uint32_t, PipelineBindingDetails> bindings,
    std::map<uint32_t, std::shared_ptr<Internal::DescriptorCache>> textureDescriptorCaches,
    const std::unordered_map<MaterialBindPoint, uint32_t> &materialBindings
)
    : device(device),
    resources(std::move(resources)),
    bindings(std::move(bindings)),
    textureDescriptorCaches(std::move(textureDescriptorCaches)) {

    auto it = materialBindings.find(MaterialBindPoint::Albedo);
    if (it != materialBindings.end()) {
        bindingMaterialAlbedo = it->second;
    }
    it = materialBindings.find(MaterialBindPoint::Normal);
    if (it != materialBindings.end()) {
        bindingMaterialNormal = it->second;
    }
}

Pipeline::~Pipeline() {
    device.destroyPipeline(resources.pipeline);

    device.destroyPipelineLayout(resources.layout);
    device.destroyDescriptorPool(resources.descriptorPool);
    for (auto layout : resources.descriptorLayouts) {
        device.destroyDescriptorSetLayout(layout);
    }
}

void Pipeline::bind(vk::CommandBuffer commandBuffer, uint32_t activeImage) {
    if (!descriptorUpdates.empty()) {
        device.updateDescriptorSets(descriptorUpdates, {});

        for (auto &update : descriptorUpdates) {
            delete update.pImageInfo;
            delete update.pBufferInfo;
        }

        descriptorUpdates.clear();
    }

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, resources.pipeline);

    // Ensure that all resources are in the correct states

    // Now bind all descriptor sets
    uint32_t setIndex = 0;
    for (auto &descriptorSets : resources.descriptorSets) {
        if (!descriptorSets.empty() && resources.autoBindSet[setIndex]) {
            vk::DescriptorSet set;

            if (descriptorSets.size() == 1) {
                set = descriptorSets[0];
            } else {
                set = descriptorSets[activeImage];
            }

            bindDescriptorSets(commandBuffer, setIndex, 1, &set, 0, nullptr);
        }
        ++setIndex;
    }
}

void Pipeline::bindDescriptorSets(
    vk::CommandBuffer commandBuffer, uint32_t firstSet,
    uint32_t descriptorSetCount, const vk::DescriptorSet *descriptorSets,
    uint32_t dynamicOffsetCount, const uint32_t *dynamicOffsets
) {
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        resources.layout,
        firstSet,
        descriptorSetCount, descriptorSets,
        dynamicOffsetCount, dynamicOffsets
    );
}

void Pipeline::bindImage(uint32_t set, uint32_t binding, const std::shared_ptr<Image> &image) {
    boundImages[binding] = image;
    auto &config = bindings[binding];

    // Update the descriptor
    auto *imageInfo = new vk::DescriptorImageInfo(
        config.sampler,
        image->imageView(),
        config.targetLayout
    );

    auto descriptorSets = resources.descriptorSets[set];
    for (auto &descriptorSet : descriptorSets) {
        descriptorUpdates.emplace_back(
            vk::WriteDescriptorSet(
                descriptorSet,
                binding,
                0,
                1,
                config.type,
                imageInfo
            )
        );
    }
}

void Pipeline::bindImage(uint32_t set, uint32_t binding, const std::shared_ptr<Image> &image, vk::Sampler sampler) {
    boundImages[binding] = image;
    auto &config = bindings[binding];

    config.sampler = sampler;

    // Update the descriptor
    auto *imageInfo = new vk::DescriptorImageInfo(
        sampler,
        image->imageView(),
        config.targetLayout
    );

    auto descriptorSets = resources.descriptorSets[set];
    for (auto &descriptorSet : descriptorSets) {
        descriptorUpdates.emplace_back(
            vk::WriteDescriptorSet(
                descriptorSet,
                binding,
                0,
                1,
                config.type,
                imageInfo
            )
        );
    }
}

void Pipeline::bindBuffer(uint32_t set, uint32_t binding, const std::shared_ptr<Buffer> &buffer) {
    boundBuffers[binding] = buffer;
    auto &config = bindings[binding];

    // Update the descriptor
    auto *bufferInfo = new vk::DescriptorBufferInfo(
        buffer->buffer(),
        0,
        buffer->getSize()
    );

    auto descriptorSets = resources.descriptorSets[set];
    for (auto &descriptorSet : descriptorSets) {
        descriptorUpdates.emplace_back(
            vk::WriteDescriptorSet(
                descriptorSet,
                binding,
                0,
                1,
                config.type,
                nullptr,
                bufferInfo
            )
        );
    }
}

void Pipeline::bindCamera(uint32_t set, uint32_t binding, RenderEngine &engine) {
    auto &config = bindings[binding];
    auto descriptorSets = resources.descriptorSets[set];

    uint32_t index = 0;
    for (auto &descriptorSet : descriptorSets) {
        auto *bufferInfo = new vk::DescriptorBufferInfo(engine.getCameraDBI(index));
        descriptorUpdates.emplace_back(
            vk::WriteDescriptorSet(
                descriptorSet,
                binding,
                0,
                1,
                config.type,
                nullptr,
                bufferInfo
            )
        );
        ++index;
    }
}

void Pipeline::bindTexture(vk::CommandBuffer commandBuffer, uint32_t binding, const Texture *texture) {
    auto descriptor = textureDescriptorCaches[binding]->get(texture);
    auto set = bindings[binding].set;
    bindDescriptorSets(commandBuffer, set, 1, &descriptor, 0, nullptr);
}

void Pipeline::bindMaterial(vk::CommandBuffer commandBuffer, const Material *material) {
    if (bindingMaterialAlbedo) {
        auto albedoTexture = material->getAlbedo();
        bindTexture(commandBuffer, *bindingMaterialAlbedo, albedoTexture);
    }

    if (bindingMaterialNormal) {
        auto normalTexture = material->getNormal();
        bindTexture(commandBuffer, *bindingMaterialNormal, normalTexture);
    }
}

void Pipeline::bindPoolImage(vk::CommandBuffer commandBuffer, uint32_t set, uint32_t binding, uint32_t index) {
    auto descriptorSets = resources.descriptorSets[set];
    bindDescriptorSets(commandBuffer, set, 1, &descriptorSets[index], 0, nullptr);
}

void Pipeline::updatePoolImage(uint32_t set, uint32_t binding, uint32_t index, const Image &image) {
    auto &config = bindings[binding];
    auto descriptorSets = resources.descriptorSets[set];

    // Update the descriptor
    auto *imageInfo = new vk::DescriptorImageInfo(
        config.sampler,
        image.imageView(),
        config.targetLayout
    );

    descriptorUpdates.emplace_back(
        vk::WriteDescriptorSet(
            descriptorSets[index],
            binding,
            0,
            1,
            config.type,
            imageInfo
        )
    );
}

void
Pipeline::updatePoolImage(uint32_t set, uint32_t binding, uint32_t index, const Image &image, vk::ImageLayout layout) {
    auto &config = bindings[binding];
    auto descriptorSets = resources.descriptorSets[set];

    // Update the descriptor
    auto *imageInfo = new vk::DescriptorImageInfo(
        config.sampler,
        image.imageView(),
        layout
    );

    descriptorUpdates.emplace_back(
        vk::WriteDescriptorSet(
            descriptorSets[index],
            binding,
            0,
            1,
            config.type,
            imageInfo
        )
    );
}

void Pipeline::updatePoolImage(uint32_t set, uint32_t binding, uint32_t index, vk::ImageView image) {
    auto &config = bindings[binding];
    auto descriptorSets = resources.descriptorSets[set];

    // Update the descriptor
    auto *imageInfo = new vk::DescriptorImageInfo(
        config.sampler,
        image,
        config.targetLayout
    );

    descriptorUpdates.emplace_back(
        vk::WriteDescriptorSet(
            descriptorSets[index],
            binding,
            0,
            1,
            config.type,
            imageInfo
        )
    );
}

void
Pipeline::updatePoolImage(uint32_t set, uint32_t binding, uint32_t index, vk::ImageView image, vk::ImageLayout layout) {
    auto &config = bindings[binding];
    auto descriptorSets = resources.descriptorSets[set];

    // Update the descriptor
    auto *imageInfo = new vk::DescriptorImageInfo(
        config.sampler,
        image,
        layout
    );

    descriptorUpdates.emplace_back(
        vk::WriteDescriptorSet(
            descriptorSets[index],
            binding,
            0,
            1,
            config.type,
            imageInfo
        )
    );
}

}
