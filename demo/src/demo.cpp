#include "demo.hpp"

#include <tech-core/subsystem/debug.hpp>
#include <tech-core/subsystem/imgui.hpp>
#include <tech-core/debug.hpp>
#include <tech-core/shapes/plane.hpp>
#include <tech-core/shapes/bounding_sphere.hpp>
#include <tech-core/post_processing.hpp>
#include <tech-core/texture/manager.hpp>
#include <tech-core/texture/builder.hpp>
#include <tech-core/material/builder.hpp>
#include <tech-core/scene/scene.hpp>
#include <tech-core/scene/debug.hpp>
#include <tech-core/scene/components/light.hpp>
#include <tech-core/scene/components/mesh_renderer.hpp>
#include <tech-core/scene/entity.hpp>
#include <tech-core/shader/standard.hpp>
#include <imgui.h>


const float AverageFPSFactor = 0.983;

void Demo::initialize() {
    engine.addSubsystem(Engine::Subsystem::DebugSubsystem::ID);
    engine.addSubsystem(Engine::Subsystem::ImGuiSubsystem::ID);

    // Initialise the engine
    engine.initialize("Demo");
    camera = std::make_unique<Engine::FPSCamera>(90, glm::vec3 { 0, 30, 20 }, 0, 0);

    camera->lookAt({ 0, 0, 0 });

    engine.setCamera(*camera);
    this->inputManager = &engine.getInputManager();
    initScene();
}

void Demo::initScene() {
    scene = std::make_shared<Engine::Scene>();
    engine.setScene(scene);

    auto testMesh = engine.createStaticMesh<Engine::Vertex>("entity.test")
        .fromModel("assets/models/builtin/cube.obj")
        .build();

    auto testMaterial = engine.createMaterial("test-material")
        .withShader(Engine::BuiltIn::StandardPipelineDSGeometryPass)
        .withTexture(
            Engine::MaterialVariables::AlbedoTexture,
            engine.createTexture("rock")
                .fromFile("assets/textures/terrain/Rock022_2K_Color.jpg")
                .withMipMaps(Engine::TextureMipType::Generate)
                .finish()
        )
        .withTexture(
            Engine::MaterialVariables::NormalTexture,
            engine.createTexture("rock_normal")
                .fromFile("assets/textures/terrain/Rock022_2K_Normal.jpg")
                .withMipMaps(Engine::TextureMipType::Generate)
                .finish()
        )
        .withUniform(
            Engine::MaterialVariables::ScaleAndOffsetUniform, {
                glm::vec2 { 10, 10 },
                glm::vec2 { 0, 0 }
            }
        )
        .build();

    auto floor = Engine::Entity::createEntity<Engine::MeshRenderer>(1);
    floor->getTransform().setPosition({});
    floor->getTransform().setScale({ 1000, 1000, 0.01 });
    floor->get<Engine::MeshRenderer>().setMesh(testMesh);
    floor->get<Engine::MeshRenderer>().setMaterial(testMaterial);

    scene->addChild(floor);

    auto sunLight = Engine::Entity::createEntity<Engine::Light>(2);
    sunLight->getTransform().setPosition({ 0, 30, 10 });
    auto &light = sunLight->get<Engine::Light>();
    light.setType(Engine::LightType::Directional);

    scene->addChild(sunLight);

    auto pointLight = Engine::Entity::createEntity<Engine::Light>(3);
    pointLight->getTransform().setPosition({ 0, 0, 10 });
    auto &light2 = pointLight->get<Engine::Light>();
    light2.setType(Engine::LightType::Point);
    light2.setRange(100);
    light2.setColor({ 0.5, 0.5, 1 });

    scene->addChild(pointLight);
}

void Demo::run() {
    lastFrameStart = std::chrono::high_resolution_clock::now();

    while (engine.beginFrame()) {
        if (this->inputManager->isPressed(Engine::Key::eEscape)) {
            break;
        }

        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeDelta = frameStart - lastFrameStart;
        lastFrameStart = frameStart;
        instantFPS = 1.0f / timeDelta.count();
        averageFPS = averageFPS * AverageFPSFactor + instantFPS * (1 - AverageFPSFactor);
        instantFrameTime = timeDelta.count();

        handleCameraMovement();

        showSceneDebugUI(*engine.getScene());

        engine.render();
    }
}


void Demo::handleCameraMovement() {
    auto &input = *this->inputManager;

    const float lookSensitivity = 0.1f;
    const float moveSensitivity = 0.3f;

    if (input.wasPressed(Engine::Key::eMouseRight)) {
        input.captureMouse();
    } else if (input.wasReleased(Engine::Key::eMouseRight)) {
        input.releaseMouse();
    }
    auto mouseDelta = input.getMouseDelta();

    // Camera Rotation
    camera->setYaw(camera->getYaw() + mouseDelta.x * lookSensitivity);
    camera->setPitch(camera->getPitch() - mouseDelta.y * lookSensitivity);

    // Movement

    glm::vec3 forwardPlane = {
        sin(glm::radians(camera->getYaw())),
        cos(glm::radians(camera->getYaw())),
        0
    };
    glm::vec3 rightPlane = {
        cos(glm::radians(camera->getYaw())),
        -sin(glm::radians(camera->getYaw())),
        0
    };

    glm::vec3 inputVector = {};

    // Get movement input
    bool hasInput = false;
    if (input.isPressed(Engine::Key::eW)) {
        inputVector = forwardPlane;
        hasInput = true;
    } else if (input.isPressed(Engine::Key::eS)) {
        inputVector = -forwardPlane;
        hasInput = true;
    }

    if (input.isPressed(Engine::Key::eD)) {
        inputVector += rightPlane;
        hasInput = true;
    } else if (input.isPressed(Engine::Key::eA)) {
        inputVector += -rightPlane;
        hasInput = true;
    }

    if (input.isPressed(Engine::Key::eSpace)) {
        inputVector.z = 1;
    }
    if (input.isPressed(Engine::Key::eLeftControl)) {
        inputVector.z = -1;
    }

    if (glm::length(inputVector) > 0) {
        float speed = moveSensitivity;

        if (inputVector.x != 0 || inputVector.y != 0) {
            glm::vec3 flatVec(inputVector);
            flatVec.z = 0;

            auto inputZ = inputVector.z;

            inputVector = glm::normalize(flatVec) * speed;
            inputVector.z = inputZ * speed;
        } else {
            inputVector.z *= speed;
        }

        camera->setPosition(camera->getPosition() + inputVector);
    }
}