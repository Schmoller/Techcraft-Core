#include "tech-core/inputmanager.hpp"
#include <imgui.h>

namespace Engine {

InputManager *InputManager::instance = nullptr;

void InputManager::initialize(GLFWwindow *window) {
    this->window = window;

    glfwSetKeyCallback(window, &InputManager::onRawKeyUpdate);
    glfwSetMouseButtonCallback(window, &InputManager::onRawMouseUpdate);
    glfwSetCharCallback(window, &InputManager::onRawCharUpdate);
    glfwSetCursorPosCallback(window, &InputManager::onRawCursorPosUpdate);
    glfwSetScrollCallback(window, &InputManager::onRawScroll);
}

bool InputManager::getKeyStatus(Key key) {
    if (pendingKeyStatus.count(key) > 0) {
        return pendingKeyStatus[key];
    }

    bool state;
    int code = static_cast<int>(key);
    if (code & MOUSE_BIT) {
        code -= MOUSE_BIT;
        if (isMouseAvailable()) {
            state = glfwGetMouseButton(window, code) == GLFW_PRESS;
        } else {
            state = false;
        }
    } else {
        if (isKeyboardAvailable()) {
            state = glfwGetKey(window, code) == GLFW_PRESS;
        } else {
            state = false;
        }
    }
    pendingKeyStatus[key] = state;
    return state;
}

bool InputManager::wasPressed(Key key) {
    bool lastState = false;
    if (keyStatus.count(key)) {
        lastState = keyStatus[key];
    }

    bool state = getKeyStatus(key);

    return state && !lastState;
}

bool InputManager::wasReleased(Key key) {
    bool lastState = false;
    if (keyStatus.count(key)) {
        lastState = keyStatus[key];
    }

    bool state = getKeyStatus(key);

    return !state && lastState;
}

bool InputManager::isPressed(Key key) {
    return getKeyStatus(key);
}

bool InputManager::isPressedImmediate(Key key) {
    int code = static_cast<int>(key);
    if (code & MOUSE_BIT) {
        code -= MOUSE_BIT;
        if (isMouseAvailable()) {
            return glfwGetMouseButton(window, code) == GLFW_PRESS;
        } else {
            return false;
        }
    } else {
        if (isKeyboardAvailable()) {
            return glfwGetKey(window, code) == GLFW_PRESS;
        } else {
            return false;
        }
    }
}

void InputManager::updateStates() {

    // Push all pending states to last states
    for (auto state : pendingKeyStatus) {
        keyStatus[state.first] = state.second;
    }

    pendingKeyStatus.clear();
    if (mouseCaptured) {
        glfwSetCursorPos(window, 0, 0);
    }

    mouseWheel = {};
}

void InputManager::captureMouse() {
    mouseCaptured = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, 0, 0);
}

void InputManager::releaseMouse() {
    mouseCaptured = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

glm::vec2 InputManager::getMousePos() {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    return glm::vec2((float) x, (float) y);
}

void InputManager::setMousePos(const glm::vec2 &pos) {
    glfwSetCursorPos(window, pos.x, pos.y);
}

glm::vec2 InputManager::getMouseDelta() {
    if (mouseCaptured && isMouseAvailable()) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        return glm::vec2((float) x, (float) y);
    } else {
        return {};
    }
}

glm::vec2 InputManager::getMouseWheel() {
    return mouseWheel;
}

void InputManager::addCallback(InputCallback inputCallback) {
    if (inputCallback) {
        callbacks.push_back(inputCallback);
    }
}

void InputManager::addTextCallback(TextInputCallback inputCallback) {
    if (inputCallback) {
        textCallbacks.push_back(inputCallback);
    }
}

void InputManager::addMouseCallback(MouseInputCallback inputCallback) {
    if (inputCallback) {
        mouseCallbacks.push_back(inputCallback);
    }
}

void InputManager::addMouseMoveCallback(MouseMoveCallback inputCallback) {
    if (inputCallback) {
        mouseMoveCallbacks.push_back(inputCallback);
    }
}

void InputManager::addScrollCallback(MouseScrollCallback inputCallback) {
    if (inputCallback) {
        scrollCallbacks.push_back(inputCallback);
    }
}

void InputManager::onKeyUpdate(int key, int scancode, int action, int modifiers) {
    // Not supporting scan-code only keys yet
    if (key == GLFW_KEY_UNKNOWN) {
        return;
    }

    Modifier mappedModifiers;
    if (modifiers & GLFW_MOD_SHIFT) {
        mappedModifiers |= ModifierFlag::Shift;
    }
    if (modifiers & GLFW_MOD_CONTROL) {
        mappedModifiers |= ModifierFlag::Control;
    }
    if (modifiers & GLFW_MOD_ALT) {
        mappedModifiers |= ModifierFlag::Alt;
    }
    if (modifiers & GLFW_MOD_SUPER) {
        mappedModifiers |= ModifierFlag::Super;
    }
    for (auto &callback : callbacks) {
        callback(
            static_cast<Key>(key),
            static_cast<Action>(action),
            mappedModifiers
        );
    }
}

void InputManager::onMouseUpdate(int button, int action, int modifiers) {
    Modifier mappedModifiers;
    if (modifiers & GLFW_MOD_SHIFT) {
        mappedModifiers |= ModifierFlag::Shift;
    }
    if (modifiers & GLFW_MOD_CONTROL) {
        mappedModifiers |= ModifierFlag::Control;
    }
    if (modifiers & GLFW_MOD_ALT) {
        mappedModifiers |= ModifierFlag::Alt;
    }
    if (modifiers & GLFW_MOD_SUPER) {
        mappedModifiers |= ModifierFlag::Super;
    }

    for (auto &callback : callbacks) {
        callback(
            static_cast<Key>(button | MOUSE_BIT),
            static_cast<Action>(action),
            mappedModifiers
        );
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    MouseButton mouseButton;

    switch (button) {
        case GLFW_MOUSE_BUTTON_1:
            mouseButton = MouseButton::Mouse1;
            break;
        case GLFW_MOUSE_BUTTON_2:
            mouseButton = MouseButton::Mouse2;
            break;
        case GLFW_MOUSE_BUTTON_3:
            mouseButton = MouseButton::Mouse3;
            break;
        case GLFW_MOUSE_BUTTON_4:
            mouseButton = MouseButton::Mouse4;
            break;
        case GLFW_MOUSE_BUTTON_5:
            mouseButton = MouseButton::Mouse5;
            break;
        case GLFW_MOUSE_BUTTON_6:
            mouseButton = MouseButton::Mouse6;
            break;
        case GLFW_MOUSE_BUTTON_7:
            mouseButton = MouseButton::Mouse7;
            break;
        case GLFW_MOUSE_BUTTON_8:
            mouseButton = MouseButton::Mouse8;
            break;
    }

    for (auto &callback : mouseCallbacks) {
        callback(
            x, y,
            static_cast<Action>(action),
            mouseButton,
            mappedModifiers
        );
    }
}

void InputManager::onCharUpdate(wchar_t ch) {
    for (auto &callback : textCallbacks) {
        callback(ch);
    }
}

void InputManager::onCursorPosUpdate(double x, double y) {
    if (mouseMoveCallbacks.empty()) {
        return;
    }

    MouseButtons buttons;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        buttons |= MouseButton::Mouse1;
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
        buttons |= MouseButton::Mouse2;
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS) {
        buttons |= MouseButton::Mouse3;
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_4) == GLFW_PRESS) {
        buttons |= MouseButton::Mouse4;
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_5) == GLFW_PRESS) {
        buttons |= MouseButton::Mouse5;
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_6) == GLFW_PRESS) {
        buttons |= MouseButton::Mouse6;
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_7) == GLFW_PRESS) {
        buttons |= MouseButton::Mouse7;
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_8) == GLFW_PRESS) {
        buttons |= MouseButton::Mouse8;
    }

    Modifier mappedModifiers;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        mappedModifiers |= ModifierFlag::Shift;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        mappedModifiers |= ModifierFlag::Control;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
        mappedModifiers |= ModifierFlag::Alt;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS) {
        mappedModifiers |= ModifierFlag::Super;
    }

    for (auto &callback : mouseMoveCallbacks) {
        callback(
            x, y,
            buttons,
            mappedModifiers
        );
    }
}

void InputManager::onScroll(double scrollX, double scrollY) {
    if (isMouseAvailable()) {
        mouseWheel.x += static_cast<float>(scrollX);
        mouseWheel.y += static_cast<float>(scrollY);

        for (auto &callback : scrollCallbacks) {
            callback(scrollX, scrollY);
        }
    }
}

void InputManager::onRawKeyUpdate(GLFWwindow *window, int key, int scancode, int action, int modifiers) {
    instance->onKeyUpdate(key, scancode, action, modifiers);
}

void InputManager::onRawMouseUpdate(GLFWwindow *window, int button, int action, int modifiers) {
    instance->onMouseUpdate(button, action, modifiers);
}

void InputManager::onRawCharUpdate(GLFWwindow *window, uint32_t codepoint) {
    instance->onCharUpdate(static_cast<wchar_t>(codepoint));
}

void InputManager::onRawCursorPosUpdate(GLFWwindow *window, double x, double y) {
    instance->onCursorPosUpdate(x, y);
}

void InputManager::onRawScroll(GLFWwindow *window, double scrollX, double scrollY) {
    instance->onScroll(scrollX, scrollY);
}

bool InputManager::isMouseAvailable() const {
    if (ImGui::GetCurrentContext()) {
        return !ImGui::GetIO().WantCaptureMouse;
    }

    return true;
}

bool InputManager::isKeyboardAvailable() const {
    if (ImGui::GetCurrentContext()) {
        return !ImGui::GetIO().WantCaptureKeyboard;
    }

    return true;
}

}