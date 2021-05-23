#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "common_includes.hpp"

#include "tech-core/shapes/frustum.hpp"

namespace Engine {

struct CameraUBO {
    glm::mat4 view;
    glm::mat4 proj;
};

class CameraDescriptorSlot {
public:
    vk::DescriptorBufferInfo bufferInfo(uint32_t imageIndex) const;
};

enum class CameraType {
    Perspective,
    Orthogonal
};

class Camera {
public:
    Camera(
        CameraType type, const glm::vec3 &position, const glm::vec3 &forward = { 0.0f, 1.0f, 0.0f },
        const glm::vec3 &up = { 0.0f, 0.0f, 1.0f }
    );
    Camera(
        float fov, const glm::vec3 &position, const glm::vec3 &forward = { 0.0f, 1.0f, 0.0f },
        const glm::vec3 &up = { 0.0f, 0.0f, 1.0f }
    );

    const CameraUBO *getUBO() const;

    void setPosition(const glm::vec3 &position);
    const glm::vec3 &getPosition() const;

    virtual void setForward(const glm::vec3 &forward);
    const glm::vec3 &getForward() const;

    void setUp(const glm::vec3 &up);
    const glm::vec3 &getUp() const;

    glm::vec3 getRight() const;

    void setType(CameraType type);
    CameraType getType() const;

    void setFOV(float fov);
    float getFOV() const;

    void setAspectRatio(float aspectRatio);
    float getAspectRatio() const;

    void setNearClip(float);

    float getNearClip() const { return nearClip; }

    void setFarClip(float);

    float getFarClip() const { return farClip; }

    virtual void lookAt(const glm::vec3 &target);

    const Frustum &getFrustum() const;

    /**
     * Creates a ray from a screen space coordinate
     * @param screenCoord The screen space coordinate in the range of -1 to 1
     * @param worldOrigin out. The position in world space of the input coordinate.
     * @param worldDirection out. The direction in world space of the input coordinate.
     */
    void rayFromCoord(const glm::vec2 &screenCoord, glm::vec3 &worldOrigin, glm::vec3 &worldDirection) const;

private:
    CameraType type;

    CameraUBO uniform;

    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 up;

    // Perspective
    float fov;
    float aspectRatio;
    float nearClip { 0.1f };
    float farClip { 10000.0f };

    Frustum frustum;

    void updateView();
    void updateProjection();
};

class FPSCamera : public Camera {
public:
    FPSCamera(float fov, const glm::vec3 &position, float yaw, float pitch, const glm::vec3 &up = { 0.0f, 0.0f, 1.0f });

    float getYaw() const { return yaw; }

    float getPitch() const { return pitch; }

    void setYaw(float yaw);
    void setPitch(float pitch);

    void setForward(const glm::vec3 &forward) override;
    void lookAt(const glm::vec3 &target) override;

private:
    float yaw;
    float pitch;
};
}

#endif