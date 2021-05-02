#pragma once

#include "texturemanager.hpp"
#include "utilities/flags.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <array>
#include <vector>
#include <functional>

#include "font.hpp"

namespace Engine::Gui {

// In lieu of including the drawer
class Drawer;

typedef uint16_t GuiBufferInt;

struct Rect {
    glm::vec2 topLeft;
    glm::vec2 bottomRight;

    inline float width() const { return bottomRight.x - topLeft.x; }
    inline float height() const { return bottomRight.y - topLeft.y; }

    inline glm::vec2 center() const {
        return {
            (topLeft.x + bottomRight.x) / 2,
            (topLeft.y + bottomRight.y) / 2
        };
    }

    inline float left() const { return topLeft.x; }
    inline float top() const { return topLeft.y; }
    inline float right() const { return bottomRight.x; }
    inline float bottom() const { return bottomRight.y; }

    bool contains(float x, float y) const;
};

struct AnchorOffsets {
    float top;
    float left;
    float bottom;
    float right;
};

enum class AnchorFlag {
    Top = 0x01,
    Left = 0x02,
    Bottom = 0x04,
    Right = 0x08,
    Center = 0x40000000
};

typedef Utilities::Flags<AnchorFlag> Anchor;

inline Anchor operator|(AnchorFlag value1, AnchorFlag value2) {
    return Anchor(value1) | value2;
}
inline Anchor operator~(AnchorFlag value1) {
    return ~Anchor(value1);
}

const Anchor AnchorAll = AnchorFlag::Top | AnchorFlag::Left | AnchorFlag::Bottom | AnchorFlag::Right;

class BaseComponent {
    friend class GuiManager;

    public:
    const Rect &getBounds() const;
    void setBounds(const Rect &bounds);

    const Anchor &getAnchor() const;
    void setAnchor(const Anchor &anchor);

    const AnchorOffsets &getAnchorOffsets() const;
    void setAnchorOffsets(const AnchorOffsets &offsets);

    void markDirty();

    void onRender(Drawer &drawer);

    virtual void onScreenResize(uint32_t width, uint32_t height);
    virtual void onLayout(const Rect &parentBounds);

    bool needsLayout() const { return layoutRequired; }
    void markNeedsLayout();

    protected:
    BaseComponent(const Rect &bounds, const Anchor &anchor = {}, const AnchorOffsets &offsets = {});
    virtual void onFrameUpdate();
    virtual void onPaint(Drawer &drawer) {};

    void addChild(std::shared_ptr<BaseComponent> component);
    void removeChild(std::shared_ptr<BaseComponent> component);

    const std::vector<std::shared_ptr<BaseComponent>> &getChildren() const;

    Rect bounds;
    Anchor anchor;
    AnchorOffsets anchorOffsets;

    private:
    void onRegister(uint16_t id, std::function<void()> markDirtyCallback);

    bool layoutRequired { true };

    std::function<void()> internalMarkDirty;
    uint16_t id;

    std::vector<std::shared_ptr<BaseComponent>> children;
    BaseComponent *parent { nullptr };
};

class Image : public BaseComponent {
    public:
    Image(const Engine::Texture *texture, const Rect &bounds);

    protected:
    void onPaint(Drawer &drawer) override;

    private:
    const Texture *texture;
};

class TextBox : public BaseComponent {
    public:
    TextBox(const std::wstring &text, const Rect &bounds, Engine::Font *font = nullptr);

    void update(const std::wstring &text);

    protected:
    void onPaint(Drawer &drawer) override;

    private:
    Engine::Font *font;
    std::wstring text;
};

}