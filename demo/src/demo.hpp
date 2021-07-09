#pragma once

#include <tech-core/camera.hpp>
#include <tech-core/engine.hpp>
#include <tech-core/scene/scene.hpp>
#include <tech-core/scene/entity.hpp>

#include <chrono>

class Demo {
public:
    void initialize();

    void run();
private:
    Engine::RenderEngine engine;

    Engine::InputManager *inputManager { nullptr };
    std::unique_ptr<Engine::FPSCamera> camera;

    std::shared_ptr<Engine::Scene> scene;

    std::chrono::high_resolution_clock::time_point lastFrameStart;
    float averageFPS { 0 };
    float instantFPS { 0 };
    float instantFrameTime { 0 };

    void initScene();
    void handleCameraMovement();
};



