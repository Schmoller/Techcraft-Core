#pragma once

#include "base.hpp"
#include "light.hpp"
#include "pipeline.hpp"

#include <vector>
#include <unordered_map>

namespace Engine::Subsystem {
namespace _E = Engine;

struct TileVertex {
    glm::vec3 pos;
    glm::vec3 tileLight;
    glm::vec3 skyLight;
    glm::vec3 occlusion;
    glm::vec3 normal;
    glm::vec3 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(
            0, 
            sizeof(TileVertex),
            vk::VertexInputRate::eVertex
        );
    }

    static std::array<vk::VertexInputAttributeDescription, 6> getAttributeDescriptions() {
        return {{
            {
                0,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(TileVertex, pos)
            },
            {
                1,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(TileVertex, tileLight)
            },
            {
                2,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(TileVertex, skyLight)
            },
            {
                3,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(TileVertex, occlusion)
            },
            {
                4,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(TileVertex, normal)
            },
            {
                5,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(TileVertex, texCoord)
            }
        }};
    }
};

/**
 * A segment of the terrain mesh.
 * NOTE: This does not need to be an entire chunk. Multiple of these could be made per chunk
 */
struct TerrainSegment {
    uint32_t id;
    // Mesh details
    _E::DivisibleBuffer *buffer;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    vk::DeviceSize indexOffset;
    uint32_t indexCount;
    vk::IndexType indexType;
    uint32_t textureArrayId;

    bool translucent;

    // Bounding box
    glm::vec3 minCorner;
    glm::vec3 maxCorner;
};

class TerrainSubsystem : public Subsystem {
    typedef std::vector<std::unique_ptr<_E::DivisibleBuffer>> BufferVec;
    typedef std::unordered_map<uint32_t, TerrainSegment> SegmentMap;

    public:
    static const SubsystemID<TerrainSubsystem> ID;

    TerrainSubsystem();

    // Public API
    uint32_t addSegment(
        const std::vector<TileVertex> &vertices,
        const std::vector<uint16_t> &indices,
        uint32_t textureArrayId,
        glm::vec3 minCorner,
        glm::vec3 maxCorner,
        bool isTranslucent = false
    );
    uint32_t updateSegment(
        uint32_t id,
        const std::vector<TileVertex> &vertices,
        const std::vector<uint16_t> &indices,
        uint32_t textureArrayId,
        glm::vec3 minCorner,
        glm::vec3 maxCorner,
        bool isTranslucent = false
    );
    void removeSegment(uint32_t id);

    // For engine use
    void initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) override;
    void initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) override;
    void cleanupResources(vk::Device device, _E::RenderEngine &engine) override;
    void cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) override;
    void writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) override;
    void prepareFrame(uint32_t activeImage) override;

    private:
    TerrainSegment &allocate(vk::DeviceSize size, bool transparent);
    void free(TerrainSegment *segment);
    void sortTriangles(bool force = false);

    // Config
    float sortThreshold { 0.25 }; // Square distance

    // Provided
    _E::TaskManager *taskManager;
    _E::BufferManager *bufferManager;
    _E::TextureManager *textureManager;
    uint32_t terrainSamplerId;
    vk::Sampler terrainSampler;
    _E::RenderEngine *engine;

    LightSubsystem *globalLight {nullptr};

    // State
    BufferVec meshBuffers;
    SegmentMap segments;
    BufferVec transparentMeshBuffers;
    SegmentMap transparentSegments;
    std::vector<TerrainSegment*> orderedTransparentSegments;
    bool requireSort { true };

    uint32_t nextId;
    glm::vec3 lastCamPos {};

    // Render state
    std::unique_ptr<_E::Pipeline> pipelineNormal;
    std::unique_ptr<_E::Pipeline> pipelineTranslucent;
    vk::DescriptorSetLayout descriptorLayout;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;
};

}