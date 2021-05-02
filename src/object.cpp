#include "object.hpp"

namespace Engine {

Object::Object(uint32_t objectId)
    : objectId(objectId),
    mesh(nullptr),
    material(nullptr),
    position({}),
    rotation(glm::quat()),
    explicitTransform(false),
    isModified(true),
    scale({1,1,1}),
    lightSize({1,1,1})
{
}

void Object::setPosition(const glm::vec3 &position) {
    this->position = position;
    isModified = true;
    explicitTransform = false;
}

glm::vec3 const &Object::getPosition() const {
    return position;
}

void Object::setRotation(const glm::quat &rotation) {
    this->rotation = rotation;
    isModified = true;
    explicitTransform = false;
}

glm::quat const &Object::getRotation() const {
    return rotation;
}

void Object::setScale(const glm::vec3 &scale) {
    this->scale = scale;
    isModified = true;
    explicitTransform = false;
}

void Object::setScale(float scale) {
    this->scale = { scale, scale, scale };
    isModified = true;
}

glm::vec3 const &Object::getScale() const {
    return this->scale;
}

void Object::setTransform(const glm::mat4 &transform) {
    this->transform = transform;
    explicitTransform = true;
    isModified = true;
}

glm::mat4 const &Object::getTransform() {
    if (isModified && !explicitTransform) {
        recomputeTransform();
    }

    return transform;
}

glm::mat4 const &Object::getTransformAndClear() {
    if (isModified && !explicitTransform) {
        recomputeTransform();
        isModified = false;
    }

    return transform;
}

void Object::setMesh(const Mesh *mesh) {
    this->mesh = mesh;
}
const Mesh *Object::getMesh() const {
    return mesh;
}

void Object::setMaterial(const Material *material) {
    this->material = material;
    isModified = true;
}
const Material *Object::getMaterial() const {
    return this->material;
}

bool Object::getIsModified() {
    return isModified;
}

void Object::setDirty() {
    isModified = true;
}

ObjectCoord const &Object::getObjCoord() const {
    return coord;
}

void Object::setObjCoord(ObjectCoord coord) {
    this->coord = coord;
}

void Object::recomputeTransform() {
    transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform *= glm::mat4_cast(rotation);
    transform = glm::scale(transform, scale);
}

uint32_t Object::getObjectId() const {
    return objectId;
}

bool Object::operator==(Object const &other) const {
    return other.objectId == objectId;
}

const glm::vec3 &Object::getLightSize() const {
    return lightSize;
}
void Object::setLightSize(const glm::vec3 &size) {
    lightSize = size;
    isModified = true;
}

void Object::setTileLight(const LightCube &light) {
    tileLight = light;
    isModified = true;
}
const LightCube &Object::getTileLight() {
    return tileLight;
}
void Object::setSkyTint(const LightCube &tint) {
    skyTint = tint;
    isModified = true;
}
const LightCube &Object::getSkyTint() {
    return skyTint;
}
void Object::setOcclusion(const LightCube &occlusion) {
    this->occlusion = occlusion;
    isModified = true;
}
const LightCube &Object::getOcclusion() {
    return occlusion;
}

}