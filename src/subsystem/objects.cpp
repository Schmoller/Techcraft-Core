#include "tech-core/subsystem/objects.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/texture/common.hpp"
#include "tech-core/texture/manager.hpp"
#include "vulkanutils.hpp"

#include <functional>
#include <iostream>

namespace Engine::Subsystem {
namespace _E = Engine;

const uint32_t maxObjectBuffers = 1024;

#define BINDING_CAMERA 0
#define BINDING_GLOBALLIGHT 1
#define BINDING_TEXTURE 2
#define BINDING_OBJECT 3

struct ObjectUBO {
    alignas(16) glm::mat4 transform;
    alignas(16) glm::vec3 offset;
    alignas(4)  uint32_t textureIndex;
    alignas(16) glm::vec3 instanceScale;
    alignas(16) LightCube tileLight;
    alignas(16) LightCube skyLight;
    alignas(16) LightCube occlusion;
};

// ObjectBuilder
ObjectBuilder::ObjectBuilder(ObjectAllocator allocator)
    : allocator(allocator) {}

ObjectBuilder &ObjectBuilder::withMesh(const _E::Mesh &mesh) {
    this->mesh = &mesh;
    return *this;
}

ObjectBuilder &ObjectBuilder::withMaterial(const _E::Material &material) {
    this->material = &material;
    return *this;
}

ObjectBuilder &ObjectBuilder::withMesh(const _E::Mesh *mesh) {
    this->mesh = mesh;
    return *this;
}

ObjectBuilder &ObjectBuilder::withMaterial(const _E::Material *material) {
    this->material = material;
    return *this;
}

ObjectBuilder &ObjectBuilder::withPosition(const glm::vec3 &pos) {
    this->position = pos;
    return *this;
}

ObjectBuilder &ObjectBuilder::withScale(const glm::vec3 &scale) {
    this->scale = scale;
    return *this;
}

ObjectBuilder &ObjectBuilder::withRotation(const glm::quat &rotation) {
    this->rotation = rotation;
    return *this;
}

ObjectBuilder &ObjectBuilder::withSize(const glm::vec3 &size) {
    this->size = size;
    return *this;
}

ObjectBuilder &ObjectBuilder::withTileLight(const LightCube &light) {
    this->tileLight = light;
    return *this;
}

ObjectBuilder &ObjectBuilder::withSkyTint(const LightCube &tint) {
    this->skyTint = tint;
    return *this;
}

ObjectBuilder &ObjectBuilder::withOcclusion(const LightCube &occlusion) {
    this->occlusion = occlusion;
    return *this;
}

std::shared_ptr<Object> ObjectBuilder::build() {
    if (!mesh || !material) {
        throw std::runtime_error("Failed to specify mesh or material for object");
    }

    auto object = allocator();
    object->setMesh(mesh);
    object->setMaterial(material);
    object->setPosition(position);
    object->setRotation(rotation);
    object->setScale(scale);
    object->setLightSize(size);
    object->setTileLight(tileLight);
    object->setSkyTint(skyTint);
    object->setOcclusion(occlusion);

    return object;
}


// ObjectSubsystem
const SubsystemID<ObjectSubsystem> ObjectSubsystem::ID;

ObjectBuilder ObjectSubsystem::createObject() {
    auto allocator = std::bind(&ObjectSubsystem::allocate, this);
    return ObjectBuilder(allocator);
}

std::shared_ptr<Object> ObjectSubsystem::getObject(uint32_t objectId) {
    if (objects.count(objectId) == 0) {
        return nullptr;
    }

    return objects[objectId];
}

void ObjectSubsystem::removeObject(uint32_t objectId) {
    std::unique_lock lock(objectLock);
    auto it = objects.find(objectId);
    if (it == objects.end()) {
        return;
    }

    // Release resources
    auto object = it->second;

    auto &buffer = objectBuffers[object->getObjCoord().bufferIndex];

    buffer.buffer->freeSection(object->getObjCoord().offset, uboBufferAlignment);

    // Remove
    objects.erase(it);
}

void ObjectSubsystem::removeObject(const std::shared_ptr<Object> &object) {
    removeObject(object->getObjectId());
}

std::shared_ptr<Object> ObjectSubsystem::allocate() {
    std::unique_lock lock(objectLock);

    uint32_t objectId = objectIdNext++;

    std::cout << "Allocate object:" << std::endl;

    auto object = std::make_shared<Object>(objectId);

    // Find the next available slot
    bool allocated = false;

    // TODO: Need a way to have the allocator consider the alignment requirements
    for (auto &buffer : objectBuffers) {
        auto offset = buffer.buffer->allocateSection(uboBufferAlignment);
        if (offset != ALLOCATION_FAILED) {
            allocated = true;
            std::cout << "Used a slot in buffer " << buffer.id << std::endl;
            object->setObjCoord({ buffer.id, offset });
            break;
        }
    }

    if (!allocated) {
        if (objectBuffers.size() >= maxObjectBuffers) {
            throw std::runtime_error("Out of object slots");
        }
        std::cout << "Need to allocate an object buffer" << std::endl;

        auto &buffer = newObjectBuffer();
        auto offset = buffer.buffer->allocateSection(uboBufferAlignment);
        // Sanity check
        if (offset == ALLOCATION_FAILED) {
            throw std::runtime_error("Out of object slots");
        }
        object->setObjCoord({ buffer.id, offset });
    }

    // Mark it as dirty so the data is copied across before rendering
    object->setDirty();

    objects[objectId] = object;
    return object;
}

ObjectSubsystem::ObjectBuffer &ObjectSubsystem::newObjectBuffer() {
    auto uboBuffer = bufferManager->aquireDivisible(
        uboBufferMaxSize,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryUsage::eCPUToGPU
    );

    auto descriptorSets = device.allocateDescriptorSets(
        {
            objectDSPool,
            1,
            &objectDSL
        }
    );

    auto objectDS = descriptorSets[0];

    // Assign buffer to DS
    vk::DescriptorBufferInfo bufferInfo(
        uboBuffer->buffer(),
        0,
        sizeof(ObjectUBO)
    );

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {{
        { // Object UBO
            objectDS,
            BINDING_OBJECT, // Binding
            0, // Array element
            1, // Count
            vk::DescriptorType::eUniformBufferDynamic,
            nullptr,
            &bufferInfo
        }
    }};

    device.updateDescriptorSets(descriptorWrites, {});

    auto &buffer = objectBuffers.emplace_back();
    buffer.id = objectBuffers.size() - 1;
    buffer.buffer = std::move(uboBuffer);
    buffer.set = objectDS;

    return buffer;
}

void ObjectSubsystem::updateObjectBuffers(bool force) {
    for (auto &pair : objects) {
        auto &obj = *pair.second;

        if (!force && !obj.getIsModified()) {
            continue;
        }

        auto coord = obj.getObjCoord();
        auto buffer = objectBuffers[coord.bufferIndex].buffer.get();

        // Copy in the updated transformation matrix
        const glm::mat4 &transform = obj.getTransformAndClear();
        auto material = obj.getMaterial();
        auto texture = material->diffuseTexture;
        buffer->copyIn(
            &transform,
            coord.offset + offsetof(ObjectUBO, transform),
            sizeof(glm::mat4)
        );
        if (texture) {
            uint32_t index = static_cast<uint32_t>(texture->arraySlot);
            buffer->copyIn(
                &index,
                coord.offset + offsetof(ObjectUBO, textureIndex),
                sizeof(uint32_t)
            );
        }
        buffer->copyIn(
            &obj.getPosition(),
            coord.offset + offsetof(ObjectUBO, offset),
            sizeof(glm::vec3)
        );
        buffer->copyIn(
            &obj.getLightSize(),
            coord.offset + offsetof(ObjectUBO, instanceScale),
            sizeof(glm::vec3)
        );
        buffer->copyIn(
            &obj.getTileLight(),
            coord.offset + offsetof(ObjectUBO, tileLight),
            sizeof(LightCube)
        );
        buffer->copyIn(
            &obj.getSkyTint(),
            coord.offset + offsetof(ObjectUBO, skyLight),
            sizeof(LightCube)
        );
        buffer->copyIn(
            &obj.getOcclusion(),
            coord.offset + offsetof(ObjectUBO, occlusion),
            sizeof(LightCube)
        );
    }

    // TODO: Buffer flushing. Make these buffers not host coherent and flush only changed regions
}

void
ObjectSubsystem::initialiseResources(vk::Device device, vk::PhysicalDevice physicalDevice, _E::RenderEngine &engine) {
    // Object shader layout contains the camera UBO
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {{
        {
            BINDING_CAMERA, // binding
            vk::DescriptorType::eUniformBuffer,
            1, // count
            vk::ShaderStageFlagBits::eVertex
        }
    }};

    cameraAndModelDSL = device.createDescriptorSetLayout(
        {
            {}, vkUseArray(bindings)
        }
    );

    // Holds the object information
    std::array<vk::DescriptorSetLayoutBinding, 1> objectBinding = {{
        {
            BINDING_OBJECT, // binding
            vk::DescriptorType::eUniformBufferDynamic,
            1, // count
            vk::ShaderStageFlagBits::eVertex
        }
    }};

    objectDSL = device.createDescriptorSetLayout(
        {
            {}, vkUseArray(objectBinding)
        }
    );

    size_t minimumAlignment = physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;
    uboBufferAlignment = sizeof(ObjectUBO);

    uboBufferAlignment = (uboBufferAlignment + minimumAlignment - 1) & ~(minimumAlignment - 1);
    uboBufferMaxSize = physicalDevice.getProperties().limits.maxUniformBufferRange;

    materialManager = &engine.getMaterialManager();
    bufferManager = &engine.getBufferManager();
    this->device = device;

    globalLight = engine.getSubsystem(LightSubsystem::ID);
}

void
ObjectSubsystem::initialiseSwapChainResources(vk::Device device, _E::RenderEngine &engine, uint32_t swapChainImages) {
    // Descriptor pool for allocating the descriptors
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {
            vk::DescriptorType::eUniformBuffer,
            swapChainImages
        }
    }};

    descriptorPool = device.createDescriptorPool(
        {
            {},
            swapChainImages,
            vkUseArray(poolSizes)
        }
    );

    // Descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(swapChainImages, cameraAndModelDSL);

    cameraAndModelDS = device.allocateDescriptorSets(
        {
            descriptorPool,
            vkUseArray(layouts)
        }
    );

    // Assign buffers to DS'
    for (uint32_t imageIndex = 0; imageIndex < swapChainImages; ++imageIndex) {
        auto cameraUbo = engine.getCameraDBI(imageIndex);

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
            vk::WriteDescriptorSet(
                cameraAndModelDS[imageIndex],
                BINDING_CAMERA, // Binding
                0, // Array element
                1, // Count
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &cameraUbo
            )
        };

        device.updateDescriptorSets(descriptorWrites, {});
    }

    initObjectBufferDSs();

    pipeline = engine.createPipeline()
        .withVertexShader("assets/shaders/vert.spv")
        .withFragmentShader("assets/shaders/frag.spv")
        .withDescriptorSet(cameraAndModelDSL)
        .withDescriptorSet(globalLight->layout())
        .withDescriptorSet(objectDSL)
        .withDescriptorSet(engine.getTextureManager().getLayout())
        .withVertexBindingDescription(Vertex::getBindingDescription())
        .withVertexAttributeDescriptions(Vertex::getAttributeDescriptions())
        .build();

    // TODO: Not sure why we need this
    updateObjectBuffers(true);
}

void ObjectSubsystem::initObjectBufferDSs() {
    std::unique_lock lock(objectLock);
    // Descriptor pool for allocating the descriptors
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {
            vk::DescriptorType::eUniformBufferDynamic,
            maxObjectBuffers
        }
    }};

    objectDSPool = device.createDescriptorPool(
        {
            {},
            maxObjectBuffers,
            vkUseArray(poolSizes)
        }
    );

    // Re-allocate descriptor sets
    if (objectBuffers.empty()) {
        // Nothing to allocate
        return;
    }

    std::vector<vk::DescriptorSetLayout> layouts(objectBuffers.size(), objectDSL);

    auto sets = device.allocateDescriptorSets(
        {
            objectDSPool,
            vkUseArray(layouts)
        }
    );

    // Assign buffers to DS'
    for (uint32_t bufferIndex = 0; bufferIndex < objectBuffers.size(); ++bufferIndex) {
        auto buf = objectBuffers[bufferIndex].buffer.get();

        vk::DescriptorBufferInfo objectUbo(
            buf->buffer(),
            0,
            sizeof(ObjectUBO)
        );

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
            vk::WriteDescriptorSet(
                sets[bufferIndex],
                BINDING_OBJECT, // Binding
                0, // Array element
                1, // Count
                vk::DescriptorType::eUniformBufferDynamic,
                nullptr,
                &objectUbo
            )
        };

        device.updateDescriptorSets(descriptorWrites, {});
        objectBuffers[bufferIndex].set = sets[bufferIndex];
    }
}

void ObjectSubsystem::cleanupResources(vk::Device device, _E::RenderEngine &engine) {
    std::unique_lock lock(objectLock);
    objects.clear();
    objectBuffers.clear();
    device.destroyDescriptorSetLayout(cameraAndModelDSL);
    device.destroyDescriptorSetLayout(objectDSL);
}

void ObjectSubsystem::cleanupSwapChainResources(vk::Device device, _E::RenderEngine &engine) {
    pipeline.reset();
    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorPool(objectDSPool);
}

void ObjectSubsystem::writeFrameCommands(vk::CommandBuffer commandBuffer, uint32_t activeImage) {
    const Mesh *lastMesh = nullptr;
    std::unique_lock lock(objectLock);

    pipeline->bind(commandBuffer);

    // Bind camera and global light
    std::array<vk::DescriptorSet, 2> globalDescriptors = {
        cameraAndModelDS[activeImage],
        globalLight->descriptor(activeImage)
    };
    pipeline->bindDescriptorSets(commandBuffer, 0, vkUseArray(globalDescriptors), 0, nullptr);

    for (auto &pair : objects) {
        auto &obj = *pair.second;
        auto mesh = obj.getMesh();

        if (!mesh) {
            continue;
        }

        if (mesh != lastMesh) {
            mesh->bind(commandBuffer);
            lastMesh = mesh;
        }

        auto &buffer = objectBuffers[obj.getObjCoord().bufferIndex];

        uint32_t dyanmicOffset = obj.getObjCoord().offset;

        std::array<vk::DescriptorSet, 2> boundDescriptors = {
            buffer.set,
            materialManager->getBinding(*obj.getMaterial()),
        };

        pipeline->bindDescriptorSets(commandBuffer, 2, vkUseArray(boundDescriptors), 1, &dyanmicOffset);
        commandBuffer.drawIndexed(mesh->getIndexCount(), 1, 0, 0, 0);
    }
}

void ObjectSubsystem::prepareFrame(uint32_t activeImage) {
    updateObjectBuffers();
}


}