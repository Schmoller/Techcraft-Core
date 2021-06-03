#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include "common_includes.hpp"

#include "utilities/flags.hpp"

#include <unordered_map>
#include <functional>

#define MOUSE_BIT 0x40000000

namespace Engine {

enum class Action {
    Press = GLFW_PRESS,
    Release = GLFW_RELEASE,
    Repeat = GLFW_REPEAT
};

enum class ModifierFlag {
    Control = 0x1,
    Shift = 0x2,
    Alt = 0x4,
    Super = 0x8
};

typedef Utilities::Flags<ModifierFlag> Modifier;

inline Modifier operator|(ModifierFlag value1, ModifierFlag value2) {
    return Modifier(value1) | value2;
}

inline Modifier operator~(ModifierFlag value1) {
    return ~Modifier(value1);
}

enum class Key {
    eSpace = GLFW_KEY_SPACE,
    eApostrophe = GLFW_KEY_APOSTROPHE,
    eComma = GLFW_KEY_COMMA,
    eMinus = GLFW_KEY_MINUS,
    ePeriod = GLFW_KEY_PERIOD,
    eSlash = GLFW_KEY_SLASH,
    e0 = GLFW_KEY_0,
    e1 = GLFW_KEY_1,
    e2 = GLFW_KEY_2,
    e3 = GLFW_KEY_3,
    e4 = GLFW_KEY_4,
    e5 = GLFW_KEY_5,
    e6 = GLFW_KEY_6,
    e7 = GLFW_KEY_7,
    e8 = GLFW_KEY_8,
    e9 = GLFW_KEY_9,
    eSemicolon = GLFW_KEY_SEMICOLON,
    eEqual = GLFW_KEY_EQUAL,
    eA = GLFW_KEY_A,
    eB = GLFW_KEY_B,
    eC = GLFW_KEY_C,
    eD = GLFW_KEY_D,
    eE = GLFW_KEY_E,
    eF = GLFW_KEY_F,
    eG = GLFW_KEY_G,
    eH = GLFW_KEY_H,
    eI = GLFW_KEY_I,
    eJ = GLFW_KEY_J,
    eK = GLFW_KEY_K,
    eL = GLFW_KEY_L,
    eM = GLFW_KEY_M,
    eN = GLFW_KEY_N,
    eO = GLFW_KEY_O,
    eP = GLFW_KEY_P,
    eQ = GLFW_KEY_Q,
    eR = GLFW_KEY_R,
    eS = GLFW_KEY_S,
    eT = GLFW_KEY_T,
    eU = GLFW_KEY_U,
    eV = GLFW_KEY_V,
    eW = GLFW_KEY_W,
    eX = GLFW_KEY_X,
    eY = GLFW_KEY_Y,
    eZ = GLFW_KEY_Z,
    eLeftBracket = GLFW_KEY_LEFT_BRACKET,
    eBackslash = GLFW_KEY_BACKSLASH,
    eRightBracket = GLFW_KEY_RIGHT_BRACKET,
    eGraveAccent = GLFW_KEY_GRAVE_ACCENT,
    eWorld1 = GLFW_KEY_WORLD_1,
    eWorld2 = GLFW_KEY_WORLD_2,
    eEscape = GLFW_KEY_ESCAPE,
    eEnter = GLFW_KEY_ENTER,
    eTab = GLFW_KEY_TAB,
    eBackspace = GLFW_KEY_BACKSPACE,
    eInsert = GLFW_KEY_INSERT,
    eDelete = GLFW_KEY_DELETE,
    eRight = GLFW_KEY_RIGHT,
    eLeft = GLFW_KEY_LEFT,
    eDown = GLFW_KEY_DOWN,
    eUp = GLFW_KEY_UP,
    ePageUp = GLFW_KEY_PAGE_UP,
    ePageDown = GLFW_KEY_PAGE_DOWN,
    eHome = GLFW_KEY_HOME,
    eEnd = GLFW_KEY_END,
    eCapsLock = GLFW_KEY_CAPS_LOCK,
    eScrollLock = GLFW_KEY_SCROLL_LOCK,
    eNumLock = GLFW_KEY_NUM_LOCK,
    ePrintScreen = GLFW_KEY_PRINT_SCREEN,
    ePause = GLFW_KEY_PAUSE,
    eF1 = GLFW_KEY_F1,
    eF2 = GLFW_KEY_F2,
    eF3 = GLFW_KEY_F3,
    eF4 = GLFW_KEY_F4,
    eF5 = GLFW_KEY_F5,
    eF6 = GLFW_KEY_F6,
    eF7 = GLFW_KEY_F7,
    eF8 = GLFW_KEY_F8,
    eF9 = GLFW_KEY_F9,
    eF10 = GLFW_KEY_F10,
    eF11 = GLFW_KEY_F11,
    eF12 = GLFW_KEY_F12,
    eF13 = GLFW_KEY_F13,
    eF14 = GLFW_KEY_F14,
    eF15 = GLFW_KEY_F15,
    eF16 = GLFW_KEY_F16,
    eF17 = GLFW_KEY_F17,
    eF18 = GLFW_KEY_F18,
    eF19 = GLFW_KEY_F19,
    eF20 = GLFW_KEY_F20,
    eF21 = GLFW_KEY_F21,
    eF22 = GLFW_KEY_F22,
    eF23 = GLFW_KEY_F23,
    eF24 = GLFW_KEY_F24,
    eF25 = GLFW_KEY_F25,
    eKp0 = GLFW_KEY_KP_0,
    eKp1 = GLFW_KEY_KP_1,
    eKp2 = GLFW_KEY_KP_2,
    eKp3 = GLFW_KEY_KP_3,
    eKp4 = GLFW_KEY_KP_4,
    eKp5 = GLFW_KEY_KP_5,
    eKp6 = GLFW_KEY_KP_6,
    eKp7 = GLFW_KEY_KP_7,
    eKp8 = GLFW_KEY_KP_8,
    eKp9 = GLFW_KEY_KP_9,
    eKpDecimal = GLFW_KEY_KP_DECIMAL,
    eKpDivide = GLFW_KEY_KP_DIVIDE,
    eKpMultiply = GLFW_KEY_KP_MULTIPLY,
    eKpSubtract = GLFW_KEY_KP_SUBTRACT,
    eKpAdd = GLFW_KEY_KP_ADD,
    eKpEnter = GLFW_KEY_KP_ENTER,
    eKpEqual = GLFW_KEY_KP_EQUAL,
    eLeftShift = GLFW_KEY_LEFT_SHIFT,
    eLeftControl = GLFW_KEY_LEFT_CONTROL,
    eLeftAlt = GLFW_KEY_LEFT_ALT,
    eLeftSuper = GLFW_KEY_LEFT_SUPER,
    eRightShift = GLFW_KEY_RIGHT_SHIFT,
    eRightControl = GLFW_KEY_RIGHT_CONTROL,
    eRightAlt = GLFW_KEY_RIGHT_ALT,
    eRightSuper = GLFW_KEY_RIGHT_SUPER,
    eMenu = GLFW_KEY_MENU,
    eMouse1 = MOUSE_BIT | GLFW_MOUSE_BUTTON_1,
    eMouse2 = MOUSE_BIT | GLFW_MOUSE_BUTTON_2,
    eMouse3 = MOUSE_BIT | GLFW_MOUSE_BUTTON_3,
    eMouse4 = MOUSE_BIT | GLFW_MOUSE_BUTTON_4,
    eMouse5 = MOUSE_BIT | GLFW_MOUSE_BUTTON_5,
    eMouse6 = MOUSE_BIT | GLFW_MOUSE_BUTTON_6,
    eMouse7 = MOUSE_BIT | GLFW_MOUSE_BUTTON_7,
    eMouse8 = MOUSE_BIT | GLFW_MOUSE_BUTTON_8,
    eMouseLast = MOUSE_BIT | GLFW_MOUSE_BUTTON_LAST,
    eMouseLeft = MOUSE_BIT | GLFW_MOUSE_BUTTON_LEFT,
    eMouseRight = MOUSE_BIT | GLFW_MOUSE_BUTTON_RIGHT,
    eMouseMiddle = MOUSE_BIT | GLFW_MOUSE_BUTTON_MIDDLE,
};

enum class MouseButton {
    Mouse1 = 0x001,
    Mouse2 = 0x002,
    Mouse3 = 0x004,
    Mouse4 = 0x008,
    Mouse5 = 0x010,
    Mouse6 = 0x020,
    Mouse7 = 0x040,
    Mouse8 = 0x080,
    MouseLeft = MouseButton::Mouse1,
    MouseRight = MouseButton::Mouse2,
    MouseMiddle = MouseButton::Mouse3
};

typedef Utilities::Flags<MouseButton> MouseButtons;

inline MouseButtons operator|(MouseButton value1, MouseButton value2) {
    return MouseButtons(value1) | value2;
}

inline MouseButtons operator~(MouseButton value1) {
    return ~MouseButtons(value1);
}

class InputManager {
public:
    typedef std::function<void(Key, Action, const Modifier &)> InputCallback;
    typedef std::function<void(wchar_t)> TextInputCallback;
    typedef std::function<void(
        double x, double y, Action action, MouseButton button, Modifier modifiers
    )> MouseInputCallback;
    typedef std::function<void(double x, double y, MouseButtons buttons, Modifier modifiers)> MouseMoveCallback;
    typedef std::function<void(double scrollX, double scrollY)> MouseScrollCallback;

    InputManager() {
        instance = this;
    }

    InputManager(const InputManager &other) = delete;
    InputManager(InputManager &&other) = delete;

    void initialize(GLFWwindow *window);

    bool wasPressed(Key key);
    bool wasReleased(Key key);
    bool isPressed(Key key);
    bool isPressedImmediate(Key key);

    void updateStates();

    void captureMouse();
    void releaseMouse();

    glm::vec2 getMousePos();
    void setMousePos(const glm::vec2 &pos);
    glm::vec2 getMouseDelta();
    glm::vec2 getMouseWheel();

    InputManager &operator=(InputManager &other) = delete;

    void addCallback(InputCallback inputCallback);
    void addTextCallback(TextInputCallback inputCallback);
    void addMouseCallback(MouseInputCallback inputCallback);
    void addMouseMoveCallback(MouseMoveCallback inputCallback);
    void addScrollCallback(MouseScrollCallback inputCallback);

private:
    GLFWwindow *window { nullptr };
    // We will cheat, not every key will be tracked
    // only keys whose state has been checked will be
    std::unordered_map<Key, bool> keyStatus;
    std::unordered_map<Key, bool> pendingKeyStatus;
    glm::vec2 mouseWheel;
    bool mouseCaptured { false };
    std::vector<InputCallback> callbacks;
    std::vector<TextInputCallback> textCallbacks;
    std::vector<MouseInputCallback> mouseCallbacks;
    std::vector<MouseMoveCallback> mouseMoveCallbacks;
    std::vector<MouseScrollCallback> scrollCallbacks;

    bool getKeyStatus(Key key);

    void onKeyUpdate(int key, int scancode, int action, int modifiers);
    void onMouseUpdate(int button, int action, int modifiers);
    void onCharUpdate(wchar_t ch);

    void onCursorPosUpdate(double x, double y);
    void onScroll(double scrollX, double scrollY);

    bool isMouseAvailable() const;
    bool isKeyboardAvailable() const;

    static void onRawKeyUpdate(GLFWwindow *window, int key, int scancode, int action, int modifiers);
    static void onRawMouseUpdate(GLFWwindow *window, int button, int action, int modifiers);
    static void onRawCharUpdate(GLFWwindow *window, uint32_t codepoint);
    static void onRawCursorPosUpdate(GLFWwindow *window, double x, double y);
    static void onRawScroll(GLFWwindow *window, double scrollX, double scrollY);
    static InputManager *instance;
};

}

#endif