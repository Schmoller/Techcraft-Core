#include "camera.hpp"

namespace Engine {

Camera::Camera(CameraType type, const glm::vec3 &position, const glm::vec3 &forward, const glm::vec3 &up)
    : type(type), position(position), forward(forward), up(up), aspectRatio(1)
{
    updateView();
    updateProjection();
}

Camera::Camera(float fov, const glm::vec3 &position, const glm::vec3 &forward, const glm::vec3 &up)
    : type(CameraType::Perspective), position(position), forward(forward), fov(fov), up(up), aspectRatio(1)
{
    updateView();
    updateProjection();
}

const CameraUBO *Camera::getUBO() const {
    return &uniform;
}

void Camera::setPosition(const glm::vec3 &position) {
    this->position = position;
    updateView();
}

const glm::vec3 &Camera::getPosition() const {
    return position;
}

void Camera::setForward(const glm::vec3 &forward) {
    this->forward = glm::normalize(forward);
    updateView();
}

const glm::vec3 &Camera::getForward() const {
    return forward;
}

void Camera::setUp(const glm::vec3 &up) {
    this->up = up;
    updateView();
}

const glm::vec3 &Camera::getUp() const {
    return up;
}

glm::vec3 Camera::getRight() const {
    return glm::normalize(glm::cross(up, forward));
}

void Camera::setType(CameraType type) {
    this->type = type;
    updateProjection();
}

CameraType Camera::getType() const {
    return type;
}

void Camera::setFOV(float fov) {
    this->fov = fov;
    updateProjection();
}

float Camera::getFOV() const {
    return fov;
}

void Camera::setAspectRatio(float aspectRatio) {
    this->aspectRatio = aspectRatio;
    updateProjection();
}

float Camera::getAspectRatio() const {
    return aspectRatio;
}

void Camera::updateView() {
    uniform.view = glm::lookAt(position, position + forward, up);
    
    frustum.update(uniform.proj * uniform.view);
}

void Camera::lookAt(const glm::vec3 &target) {
    forward = glm::normalize(target - position);
    updateView();
}

void Camera::updateProjection() {
    if (type == CameraType::Perspective) {
        uniform.proj = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 10000.0f);
        uniform.proj[1][1] *= -1;

        frustum.update(uniform.proj * uniform.view);
    } else {
        // TODO: Ortho projection
    }
}

const Frustum &Camera::getFrustum() const {
    return frustum;
}

}