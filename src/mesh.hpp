#ifndef __MESH_HPP
#define __MESH_HPP

#include "vertex.hpp"
#include "buffer.hpp"
#include "task.hpp"
#include "model.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <memory>

namespace Engine {

#define VERTEX_ALIGN 4

// Forward declarations
class Mesh;
class StaticMesh;
template <typename>
class DynamicMesh;

template <typename VertexType>
class StaticMeshBuilder {
    friend class RenderEngine;

    public:
    StaticMeshBuilder &withVertices(const std::vector<VertexType> &vertices);
    StaticMeshBuilder &withIndices(const std::vector<uint32_t> &indices);
    StaticMeshBuilder &withIndices(const std::vector<uint16_t> &indices);
    StaticMeshBuilder &fromModel(const std::string &path);
    StaticMeshBuilder &fromModel(const Model &model);
    StaticMeshBuilder &fromModel(const Model &model, const std::string &subModel);

    StaticMesh *build();

    private:
    StaticMeshBuilder(
        BufferManager &bufferManager,
        TaskManager &taskManager,
        std::function<void(std::unique_ptr<StaticMesh> &)>
    );

    // Non-configurable
    BufferManager &bufferManager;
    TaskManager &taskManager;
    std::function<void(std::unique_ptr<StaticMesh> &)> registerCallback;

    // Configurable
    std::vector<VertexType> vertices;

    std::vector<uint32_t> indices32;
    std::vector<uint16_t> indices16;
    size_t indexCount;
    vk::IndexType indexType;
};

class Mesh {
    public:
    virtual ~Mesh() {}
    virtual uint32_t getIndexCount() const = 0;
    virtual vk::IndexType getIndexType() const = 0;

    virtual void bind(vk::CommandBuffer commandBuffer) const = 0;
};

class StaticMesh : public Mesh {
    template <typename>
    friend class StaticMeshBuilder;

    public:
    virtual ~StaticMesh();

    virtual uint32_t getIndexCount() const {
        return indexCount;
    }
    virtual vk::IndexType getIndexType() const {
        return indexType;
    }

    virtual void bind(vk::CommandBuffer commandBuffer) const;

    private:
    StaticMesh(
        BufferManager& bufferManager,
        std::unique_ptr<Buffer> &combinedBuffer,
        vk::DeviceSize vertexOffset,
        vk::DeviceSize indexOffset,
        uint32_t indicesCount,
        vk::IndexType indexType
    );

    BufferManager& bufferManager;

    std::unique_ptr<Buffer> combinedBuffer;
    vk::DeviceSize vertexOffset;
    vk::DeviceSize indexOffset;
    const uint32_t indexCount;
    const vk::IndexType indexType;
};

template <typename VertexType>
class DynamicMeshBuilder {
    friend class RenderEngine;
    
    public:
    DynamicMeshBuilder &withInitialVertexCapacity(uint32_t capacity);
    DynamicMeshBuilder &withInitialIndexCapacity(uint32_t capacity);

    DynamicMeshBuilder &withGrowing(uint32_t vertexChunks, uint32_t indexChunks);
    DynamicMeshBuilder &withShinking(uint32_t minimumReclaimSize);

    DynamicMeshBuilder &withMaximumVertexCapacity(uint32_t capacity);
    DynamicMeshBuilder &withMaximumIndexCapacity(uint32_t capacity);

    DynamicMesh<VertexType> *build();

    private:
    DynamicMeshBuilder(
        BufferManager&,
        TaskManager &,
        std::function<void(std::unique_ptr<DynamicMesh<VertexType>> &)>
    );

    // Non-configurable
    BufferManager& bufferManager;
    TaskManager &taskManager;
    std::function<void(std::unique_ptr<DynamicMesh<VertexType>> &)> registerCallback;

    // Configurable parameters
    vk::DeviceSize vertexBufferSize;
    vk::DeviceSize indexBufferSize;
    
    vk::DeviceSize vertexBufferMaxSize;
    vk::DeviceSize indexBufferMaxSize;

    vk::DeviceSize vertexGrowSize;
    vk::DeviceSize indexGrowSize;

    vk::DeviceSize reclaimSize;
};

typedef uint16_t DynMeshSize;

template <typename VertexType>
class DynamicMesh : public Mesh {
    friend DynamicMesh *DynamicMeshBuilder<VertexType>::build();

    public:
    virtual ~DynamicMesh();
    /**
     * Replaces all vertices and indices with the provided data
     */
    bool replaceAll(const std::vector<VertexType> &vertices, const std::vector<DynMeshSize> &indices);
    /**
     * Allows the replacement of a range of vertices
     */
    bool replaceVertices(uint32_t srcOffset, uint32_t dstOffset, uint32_t count, VertexType *vertices);
    /**
     * Allows the replacement of a range of indices
     */
    bool replaceIndices(uint32_t srcOffset, uint32_t dstOffset, uint32_t count, DynMeshSize *indices);

    uint32_t getVertexCapacity() const {
        return vertexCapacity / sizeof(VertexType);
    }

    uint32_t getIndexCapacity() const {
        return indexCapacity / sizeof(DynMeshSize);
    }

    virtual uint32_t getIndexCount() const {
        return indexCount;
    }

    virtual vk::IndexType getIndexType() const {
        return vk::IndexType::eUint16;
    }

    virtual void bind(vk::CommandBuffer commandBuffer) const override;

    private:
    DynamicMesh(
        BufferManager &bufferManager,
        TaskManager &taskManager,
        vk::DeviceSize vertexCapacity,
        vk::DeviceSize indexCapacity,
        vk::DeviceSize vertexMaxCapacity,
        vk::DeviceSize indexMaxCapacity,
        vk::DeviceSize vertexGrowSize,
        vk::DeviceSize indexGrowSize,
        vk::DeviceSize reclaimSize
    );

    void reallocate(vk::DeviceSize totalAllocationSize);

    // Provided
    BufferManager &bufferManager;
    TaskManager &taskManager;

    // Current state
    std::unique_ptr<Buffer> combinedBuffer;
    vk::DeviceSize vertexCapacity;
    vk::DeviceSize indexCapacity;
    vk::DeviceSize indexOffset;
    vk::DeviceSize totalCapacity;
    uint32_t indexCount;
    
    // Reallocation Settings
    vk::DeviceSize vertexMaxCapacity;
    vk::DeviceSize indexMaxCapacity;

    vk::DeviceSize vertexGrowSize;
    vk::DeviceSize indexGrowSize;

    vk::DeviceSize reclaimSize;
};


template <typename VertexType>
StaticMeshBuilder<VertexType>::StaticMeshBuilder(
    BufferManager &bufferManager,
    TaskManager &taskManager,
    std::function<void(std::unique_ptr<StaticMesh> &)> registerCallback
) : bufferManager(bufferManager),
    taskManager(taskManager),
    registerCallback(registerCallback),
    indexCount(0),
    indexType(vk::IndexType::eNoneNV)
{}

template <typename VertexType>
StaticMeshBuilder<VertexType> &StaticMeshBuilder<VertexType>::withVertices(const std::vector<VertexType> &vertices) {
    this->vertices = vertices;

    return *this;
}

template <typename VertexType>
StaticMeshBuilder<VertexType> &StaticMeshBuilder<VertexType>::withIndices(const std::vector<uint32_t> &indices) {
    this->indices32 = indices;
    indexCount = indices.size();
    indexType = vk::IndexType::eUint32;

    return *this;
}

template <typename VertexType>
StaticMeshBuilder<VertexType> &StaticMeshBuilder<VertexType>::withIndices(const std::vector<uint16_t> &indices) {
    this->indices16 = indices;
    indexCount = indices.size();
    indexType = vk::IndexType::eUint16;

    return *this;
}

template <typename VertexType>
StaticMeshBuilder<VertexType> &StaticMeshBuilder<VertexType>::fromModel(const std::string &path) {
    loadModel(path, *this);

    return *this;
}

template <typename VertexType>
StaticMeshBuilder<VertexType> &StaticMeshBuilder<VertexType>::fromModel(const Model &model) {
    model.applyCombined(*this);
    return *this;
}

template <typename VertexType>
StaticMeshBuilder<VertexType> &StaticMeshBuilder<VertexType>::fromModel(const Model &model, const std::string &subModel) {
    model.applySubModel(*this, subModel);
    return *this;
}

template <typename VertexType>
StaticMesh *StaticMeshBuilder<VertexType>::build() {
    if (indexCount == 0 || vertices.size() == 0 || indexType == vk::IndexType::eNoneNV) {
        throw std::runtime_error("Incomplete mesh definition");
    }

    // Create a buffer to contain both the vertices and indices
    vk::DeviceSize vertexSize = sizeof(vertices[0]) * vertices.size();
    vk::DeviceSize indexSize;
    if (indexType == vk::IndexType::eUint16) {
        indexSize = sizeof(uint16_t) * indexCount;
    } else {
        indexSize = sizeof(uint32_t) * indexCount;
    }

    // Ensure proper alignment of the indices
    vk::DeviceSize indexOffset = ((vertexSize + VERTEX_ALIGN - 1) / VERTEX_ALIGN) * VERTEX_ALIGN;
    
    vk::DeviceSize totalBufferSize = indexOffset + indexSize;

    // Stage the data ready for transfer to the GPU
    auto staging = bufferManager.aquireStaging(totalBufferSize);

    staging->copyIn(vertices.data(), vertexSize);
    if (indexType == vk::IndexType::eUint16) {
        staging->copyIn(indices16.data(), indexOffset, indexSize);
    } else {
        staging->copyIn(indices32.data(), indexOffset, indexSize);
    }

    // Prepare GPU
    std::unique_ptr<Buffer> gpuBuffer = bufferManager.aquire(
        totalBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryUsage::eGPUOnly
    );

    auto task = taskManager.createTask();

    task->execute([&, gpuBuffRef = gpuBuffer.get()](auto commandBuffer) {
        staging->transfer(commandBuffer, *gpuBuffRef);
    });

    auto fence = taskManager.submitTask(std::move(task));
    bufferManager.release(staging, fence);

    std::unique_ptr<StaticMesh> mesh(new StaticMesh(
        bufferManager,
        gpuBuffer,
        0,
        indexOffset,
        static_cast<uint32_t>(indexCount),
        indexType
    ));

    // Register with the engine
    StaticMesh *meshInst = mesh.get();
    registerCallback(mesh);

    return meshInst;
}


template <typename VertexType>
DynamicMeshBuilder<VertexType>::DynamicMeshBuilder(
    BufferManager &bufferManager,
    TaskManager &taskManager,
    std::function<void(std::unique_ptr<DynamicMesh<VertexType>> &)> registerCallback
) : bufferManager(bufferManager),
    taskManager(taskManager),
    registerCallback(registerCallback),
    vertexBufferSize(0),
    indexBufferSize(0),
    vertexBufferMaxSize(0),
    indexBufferMaxSize(0),
    vertexGrowSize(0),
    indexGrowSize(0),
    reclaimSize(0)
{}

template <typename VertexType>
DynamicMeshBuilder<VertexType> &DynamicMeshBuilder<VertexType>::withInitialVertexCapacity(uint32_t capacity) {
    vertexBufferSize = capacity * sizeof(VertexType);
    return *this;
}

template <typename VertexType>
DynamicMeshBuilder<VertexType> &DynamicMeshBuilder<VertexType>::withInitialIndexCapacity(uint32_t capacity) {
    indexBufferSize = capacity * sizeof(DynMeshSize);
    return *this;
}

template <typename VertexType>
DynamicMeshBuilder<VertexType> &DynamicMeshBuilder<VertexType>::withGrowing(uint32_t vertexChunks, uint32_t indexChunks) {
    vertexGrowSize = vertexChunks * sizeof(VertexType);
    indexGrowSize = indexChunks * sizeof(DynMeshSize);
    return *this;
}

template <typename VertexType>
DynamicMeshBuilder<VertexType> &DynamicMeshBuilder<VertexType>::withShinking(uint32_t minimumReclaimSize) {
    reclaimSize = minimumReclaimSize;
    return *this;
}

template <typename VertexType>
DynamicMeshBuilder<VertexType> &DynamicMeshBuilder<VertexType>::withMaximumVertexCapacity(uint32_t capacity) {
    vertexBufferMaxSize = capacity * sizeof(VertexType);
    return *this;
}

template <typename VertexType>
DynamicMeshBuilder<VertexType> &DynamicMeshBuilder<VertexType>::withMaximumIndexCapacity(uint32_t capacity) {
    indexBufferMaxSize = capacity * sizeof(DynMeshSize);
    return *this;
}

template <typename VertexType>
DynamicMesh<VertexType> *DynamicMeshBuilder<VertexType>::build() {
    if (vertexBufferSize == 0 || indexBufferSize == 0) {
        throw std::runtime_error("Incomplete mesh definition");
    }

    std::unique_ptr<DynamicMesh<VertexType>> mesh(new DynamicMesh<VertexType>(
        bufferManager,
        taskManager,
        vertexBufferSize,
        indexBufferSize,
        vertexBufferMaxSize,
        indexBufferMaxSize,
        vertexGrowSize,
        indexGrowSize,
        reclaimSize
    ));

    // Register with the engine
    DynamicMesh<VertexType> *meshInst = mesh.get();
    registerCallback(mesh);

    return meshInst;
}


template <typename VertexType>
DynamicMesh<VertexType>::DynamicMesh(
    BufferManager &bufferManager,
    TaskManager &taskManager,
    vk::DeviceSize vertexCapacity,
    vk::DeviceSize indexCapacity,
    vk::DeviceSize vertexMaxCapacity,
    vk::DeviceSize indexMaxCapacity,
    vk::DeviceSize vertexGrowSize,
    vk::DeviceSize indexGrowSize,
    vk::DeviceSize reclaimSize
) : bufferManager(bufferManager),
    taskManager(taskManager),
    vertexCapacity(vertexCapacity),
    indexCapacity(indexCapacity),
    vertexMaxCapacity(vertexMaxCapacity),
    indexMaxCapacity(indexMaxCapacity),
    vertexGrowSize(vertexGrowSize),
    indexGrowSize(indexGrowSize),
    reclaimSize(reclaimSize)
{
    vk::DeviceSize newIndexOffset = ((vertexCapacity + VERTEX_ALIGN - 1) / VERTEX_ALIGN) * VERTEX_ALIGN;
    vk::DeviceSize totalAllocationSize = newIndexOffset + indexCapacity;
    indexOffset = newIndexOffset;
    indexCount = 0;

    reallocate(totalAllocationSize);
}

template <typename VertexType>
DynamicMesh<VertexType>::~DynamicMesh() {
    bufferManager.releaseAfterFrame(std::move(combinedBuffer));
}

template <typename VertexType>
bool DynamicMesh<VertexType>::replaceAll(const std::vector<VertexType> &vertices, const std::vector<DynMeshSize> &indices) {
    vk::DeviceSize newVertexUsage = vertices.size() * sizeof(VertexType);
    vk::DeviceSize newIndexUsage = indices.size() * sizeof(DynMeshSize);

    vk::DeviceSize vertexAllocateSize = 0;
    vk::DeviceSize indexAllocateSize = 0;
    vk::DeviceSize vertexReclaim = 0;
    vk::DeviceSize indexReclaim = 0;

    if (newVertexUsage > vertexCapacity) {
        if (vertexGrowSize == 0) {
            // Cannot grow
            return false;
        }

        vertexAllocateSize = ((newVertexUsage + vertexGrowSize - 1) / vertexGrowSize) * vertexGrowSize;
        if (vertexAllocateSize > vertexMaxCapacity) {
            // Cannot grow
            return false;
        }
    } else if (vertexCapacity - newVertexUsage > reclaimSize && reclaimSize > 0) {
        // Need to reclaim
        vertexReclaim = ((newVertexUsage + reclaimSize - 1) / reclaimSize) * reclaimSize;
    }

    if (newIndexUsage > indexCapacity) {
        if (indexGrowSize == 0) {
            // Cannot grow
            return false;
        }

        indexAllocateSize = ((newIndexUsage + indexGrowSize - 1) / indexGrowSize) * indexGrowSize;
        if (indexAllocateSize > indexMaxCapacity) {
            // Cannot grow
            return false;
        }
    } else if (indexCapacity - newIndexUsage > reclaimSize && reclaimSize > 0) {
        // Need to reclaim
        indexReclaim = ((newIndexUsage + reclaimSize - 1) / reclaimSize) * reclaimSize;
    }

    if (vertexAllocateSize || indexAllocateSize || vertexReclaim || indexReclaim) {
        vk::DeviceSize newIndexOffset;
        if (vertexAllocateSize) {
            newIndexOffset = ((vertexAllocateSize + VERTEX_ALIGN - 1) / VERTEX_ALIGN) * VERTEX_ALIGN;
            vertexCapacity = vertexAllocateSize;
        } else if (vertexReclaim) {
            newIndexOffset = ((vertexReclaim + VERTEX_ALIGN - 1) / VERTEX_ALIGN) * VERTEX_ALIGN;
            vertexCapacity = vertexReclaim;
        } else {
            newIndexOffset = indexOffset;
        }

        vk::DeviceSize totalAllocationSize;
        if (indexAllocateSize) {
            totalAllocationSize = newIndexOffset + indexAllocateSize;
            indexCapacity = indexAllocateSize;
        } else if (indexReclaim) {
            totalAllocationSize = newIndexOffset + indexReclaim;
            indexCapacity = indexReclaim;
        } else {
            totalAllocationSize = newIndexOffset + indexCapacity;
        }

        // If we are shinking one side but growing the other, it might be 
        // possible to fit the result in the existing buffer?
        if (totalAllocationSize > totalCapacity || (totalCapacity - totalAllocationSize > reclaimSize && reclaimSize > 0)) {
            reallocate(totalAllocationSize);
        }

        indexOffset = newIndexOffset;
    }

    // Transfer contents over
    auto staging = bufferManager.aquireStaging(totalCapacity);

    auto task = taskManager.createTask();

    staging->copyIn(vertices.data(), newVertexUsage);
    staging->copyIn(indices.data(), indexOffset, newIndexUsage);

    task->execute([&](vk::CommandBuffer commandBuffer) {
        staging->transfer(commandBuffer, *combinedBuffer, totalCapacity);
    });

    task->addMemoryBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::AccessFlagBits::eTransferWrite,
        vk::PipelineStageFlagBits::eVertexInput,
        vk::AccessFlagBits::eVertexAttributeRead
    );

    auto fence = taskManager.submitTask(std::move(task));
    bufferManager.release(staging, fence);

    indexCount = indices.size();

    return true;
}

template <typename VertexType>
bool DynamicMesh<VertexType>::replaceVertices(uint32_t srcOffset, uint32_t dstOffset, uint32_t count, VertexType *vertices) {
    vk::DeviceSize newVertexUsage = (dstOffset + count) * sizeof(VertexType);

    if (newVertexUsage > vertexCapacity) {
        // Growth not permitted when using this method
        return false;
    }

    vk::DeviceSize targetVertexSize = count * sizeof(VertexType);
    vk::DeviceSize targetOffset = dstOffset * sizeof(VertexType);

    // Transfer contents over
    auto staging = bufferManager.aquireStaging(targetVertexSize);

    auto task = taskManager.createTask();

    staging->copyIn(vertices, targetVertexSize);

    task->execute([&](auto commandBuffer) {
        staging->transfer(commandBuffer, *combinedBuffer, targetOffset, targetVertexSize);
    });

    auto fence = taskManager.submitTask(std::move(task));
    bufferManager.release(staging, fence);

    return true;
}

template <typename VertexType>
bool DynamicMesh<VertexType>::replaceIndices(uint32_t srcOffset, uint32_t dstOffset, uint32_t count, DynMeshSize *indices) {
    vk::DeviceSize newIndexUsage = (dstOffset + count) * sizeof(DynMeshSize);

    if (newIndexUsage > indexCapacity) {
        // Growth not permitted when using this method
        return false;
    }

    vk::DeviceSize targetIndexSize = count * sizeof(DynMeshSize);
    vk::DeviceSize targetOffset = dstOffset * sizeof(DynMeshSize);

    // Transfer contents over
    auto staging = bufferManager.aquireStaging(targetIndexSize);

    auto task = taskManager.createTask();

    staging->copyIn(indices, targetIndexSize);

    task->execute([&](auto commandBuffer) {
        staging->transfer(commandBuffer, *combinedBuffer, targetOffset, targetIndexSize);
    });

    auto fence = taskManager.submitTask(std::move(task));
    bufferManager.release(staging, fence);

    return true;
}

template <typename VertexType>
void DynamicMesh<VertexType>::bind(vk::CommandBuffer commandBuffer) const {
    vk::DeviceSize vertexOffset = 0;
    commandBuffer.bindVertexBuffers(0, 1, combinedBuffer->bufferArray(), &vertexOffset);
    commandBuffer.bindIndexBuffer(combinedBuffer->buffer(), indexOffset, vk::IndexType::eUint16);
}

template <typename VertexType>
void DynamicMesh<VertexType>::reallocate(vk::DeviceSize totalAllocationSize) {
    if (combinedBuffer) {
        // Ensure that the buffer is not in use
        bufferManager.releaseAfterFrame(std::move(combinedBuffer));
    }

    combinedBuffer = bufferManager.aquire(
        totalAllocationSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryUsage::eGPUOnly
    );

    totalCapacity = totalAllocationSize;
}

}

#endif