#include "tech-core/mesh.hpp"
#include "tech-core/model.hpp"

#include <stdexcept>
#include <unordered_map>
#include <iostream>

namespace Engine {

StaticMesh::StaticMesh(
    BufferManager& bufferManager,
    std::unique_ptr<Buffer> &combinedBuffer,
    vk::DeviceSize vertexOffset,
    vk::DeviceSize indexOffset,
    uint32_t indicesCount,
    vk::IndexType indexType
) : bufferManager(bufferManager),
    combinedBuffer(std::move(combinedBuffer)), 
    vertexOffset(vertexOffset),
    indexOffset(indexOffset),
    indexCount(indicesCount),
    indexType(indexType)
{}

StaticMesh::~StaticMesh() {
    bufferManager.releaseAfterFrame(std::move(combinedBuffer));
}

void StaticMesh::bind(vk::CommandBuffer commandBuffer) const {
    commandBuffer.bindVertexBuffers(0, 1, combinedBuffer->bufferArray(), &vertexOffset);
    commandBuffer.bindIndexBuffer(combinedBuffer->buffer(), indexOffset, indexType);
}


}