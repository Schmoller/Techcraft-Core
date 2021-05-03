#include "tech-core/gui/common.hpp"
#include "tech-core/gui/drawer.hpp"

namespace Engine::Gui {

// Rect
bool Rect::contains(float x, float y) const {
    return (x >= left() && y >= top() && x <= right() && y <= bottom());
}

// BaseComponent
BaseComponent::BaseComponent(const Rect &bounds, const Anchor &anchor, const AnchorOffsets &offsets)
    : bounds(bounds), anchor(anchor), anchorOffsets(offsets)
{}

const Rect &BaseComponent::getBounds() const {
    return bounds;
}

void BaseComponent::setBounds(const Rect &bounds) {
    this->bounds = bounds;
    layoutRequired = true;
    markDirty();
}

const Anchor &BaseComponent::getAnchor() const {
    return anchor;
}

void BaseComponent::setAnchor(const Anchor &anchor) {
    this->anchor = anchor;
    markDirty();
}

const AnchorOffsets &BaseComponent::getAnchorOffsets() const {
    return anchorOffsets;
}

void BaseComponent::setAnchorOffsets(const AnchorOffsets &offsets) {
    anchorOffsets = offsets;
    markDirty();
}

void BaseComponent::markDirty() {
    if (internalMarkDirty) {
        internalMarkDirty();
    }
}

void BaseComponent::markNeedsLayout() {
    layoutRequired = true;
}

void BaseComponent::onRegister(uint16_t id, std::function<void()> markDirtyCallback) {
    this->id = id;
    internalMarkDirty = markDirtyCallback;
}


void BaseComponent::onScreenResize(uint32_t width, uint32_t height) {

}
void BaseComponent::onLayout(const Rect &parentBounds) {
    layoutRequired = false;
    bool changed = false;

    if (anchor & AnchorFlag::Center) {
        float newX = (parentBounds.width() - bounds.width()) / 2 + parentBounds.left();
        if (bounds.topLeft.x != newX) {
            bounds.bottomRight.x = newX + bounds.width();
            bounds.topLeft.x = newX;
            changed = true;
        }

        float newY = (parentBounds.height() - bounds.height()) / 2 + parentBounds.top();
        if (bounds.topLeft.y != newY) {
            bounds.bottomRight.y = newY + bounds.height();
            bounds.topLeft.y = newY;
            changed = true;
        }
    }

    if (anchor & AnchorFlag::Top) {
        float newY = parentBounds.top() + anchorOffsets.top;
        if (bounds.topLeft.y != newY) {
            bounds.topLeft.y = newY;
            changed = true;
        }
    }
    if (anchor & AnchorFlag::Left) {
        float newX = parentBounds.left() + anchorOffsets.left;
        if (bounds.topLeft.x != newX) {
            bounds.topLeft.x = newX;
            changed = true;
        }
    }
    if (anchor & AnchorFlag::Bottom) {
        float newY = parentBounds.bottom() - anchorOffsets.bottom;
        if (bounds.bottomRight.y != newY) {
            bounds.bottomRight.y = newY;
            changed = true;
        }
    }
    if (anchor & AnchorFlag::Right) {
        float newX = parentBounds.right() - anchorOffsets.right;
        if (bounds.bottomRight.x != newX) {
            bounds.bottomRight.x = newX;
            changed = true;
        }
    }

    if (changed) {
        for (auto &child : children) {
            child->onLayout(bounds);
        }

        markDirty();
    }
}
void BaseComponent::onRender(Drawer &drawer) {
    onPaint(drawer);

    drawer.pushTransform();
    drawer.translate(bounds.left(), bounds.top());
    for (auto &child : children) {
        child->onRender(drawer);
    }
    drawer.popTransform();
}

void BaseComponent::onFrameUpdate() {
    for (auto &child : children) {
        if (child->needsLayout()) {
            child->onLayout(bounds);
        }

        child->onFrameUpdate();
    }
}


void BaseComponent::addChild(std::shared_ptr<BaseComponent> component) {
    children.push_back(component);
    component->onLayout(bounds);
    // TODO: It would be nice to only re-render that which changed, not entire thing
    component->internalMarkDirty = std::bind(&BaseComponent::markDirty, this);

    markDirty();
}

void BaseComponent::removeChild(std::shared_ptr<BaseComponent> component) {
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (component == *it) {
            children.erase(it);
            return;
        }
    }

    markDirty();
}

const std::vector<std::shared_ptr<BaseComponent>> &BaseComponent::getChildren() const {
    return children;
}


Image::Image(const Engine::Texture *texture, const Rect &bounds)
    : BaseComponent(bounds), texture(texture)
{}

void Image::onPaint(Drawer &drawer) {
    drawer.drawRect(bounds, *texture);
}


TextBox::TextBox(const std::wstring &text, const Rect &bounds, Engine::Font *font) 
    : BaseComponent(bounds), font(font), text(text)
{}

void TextBox::onPaint(Drawer &drawer) {
    if (font) {
        font->draw(text, drawer, bounds.topLeft);
    } else {
        drawer.drawTextWithFormatting(text, bounds.topLeft.x, bounds.topLeft.y);
    }
}

void TextBox::update(const std::wstring &text) {
    this->text = text;
    markDirty();
}

}