#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "setup.hpp"
#include "mesh.hpp"
#include "tech-core/material.hpp"

namespace Engine {

struct ObjectCoord {
    uint32_t bufferIndex;
    vk::DeviceSize offset;
};

struct LightCube {
    alignas(16) glm::vec3 westSouthDown {1,1,1};
    alignas(16) glm::vec3 westSouthUp {1,1,1};
    alignas(16) glm::vec3 westNorthDown {1,1,1};
    alignas(16) glm::vec3 westNorthUp {1,1,1};
    alignas(16) glm::vec3 eastSouthDown {1,1,1};
    alignas(16) glm::vec3 eastSouthUp {1,1,1};
    alignas(16) glm::vec3 eastNorthDown {1,1,1};
    alignas(16) glm::vec3 eastNorthUp {1,1,1};
};

class Object {
    public:
    Object(uint32_t objectId);

    void setPosition(const glm::vec3 &position);
    glm::vec3 const &getPosition() const;

    void setRotation(const glm::quat &rotation);
    glm::quat const &getRotation() const;

    void setScale(const glm::vec3 &scale);
    void setScale(float scale);
    glm::vec3 const &getScale() const;

    void setTransform(const glm::mat4 &transform);

    glm::mat4 const &getTransform();
    glm::mat4 const &getTransformAndClear();

    void setMesh(const Mesh *mesh);
    const Mesh *getMesh() const;

    void setMaterial(const Material *material);
    const Material *getMaterial() const;

    bool getIsModified();
    void setDirty();

    ObjectCoord const &getObjCoord() const;
    void setObjCoord(ObjectCoord coord);

    uint32_t getObjectId() const;

    const glm::vec3 &getLightSize() const;
    void setLightSize(const glm::vec3 &size);

    void setTileLight(const LightCube &light);
    const LightCube &getTileLight();
    void setSkyTint(const LightCube &tint);
    const LightCube &getSkyTint();
    void setOcclusion(const LightCube &occlusion);
    const LightCube &getOcclusion();

    bool operator==(Object const &other) const;

    private:
    uint32_t objectId;

    ObjectCoord coord;
    const Mesh *mesh;
    const Material *material;

    // Transformation
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    bool isModified;
    glm::mat4 transform;
    bool explicitTransform;

    // Lighting
    glm::vec3 lightSize;
    LightCube tileLight;
    LightCube skyTint;
    LightCube occlusion;

    void recomputeTransform();
};

}

#endif