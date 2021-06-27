#include "tech-core/scene/entity.hpp"
#include "render_planner.hpp"

namespace Engine {

Entity::Entity(EntityId id)
    : id(id),
    transform(*this) {

}

Entity::~Entity() {

}

void Entity::setScene(Badge<Scene>, Scene *newScene) {
    scene = newScene;
}

std::shared_ptr<Entity> Entity::getChildById(EntityId childId) const {
    for (auto &child : children) {
        if (child->id == childId) {
            return child;
        }
    }

    return {};
}

std::shared_ptr<Entity> Entity::getChildByIndex(uint32_t index) const {
    assert(index < children.size());
    return children[index];
}

void Entity::forEachChild(bool allDescendants, const std::function<void(Entity *)> &callback) const {
    for (auto &child : children) {
        callback(child.get());
        if (allDescendants) {
            child->forEachChild(allDescendants, callback);
        }
    }
}

void Entity::addChild(const std::shared_ptr<Entity> &entity) {
    assert(!entity->getParent());

    entity->parent = this;
    children.push_back(entity);
    if (scene) {
        scene->onAdd({}, entity);
    }
}

void Entity::removeChildById(EntityId childId) {
    auto it = children.begin();
    while (it != children.end()) {
        auto &child = *it;
        if (child->id == childId) {
            children.erase(it);
            child->parent = nullptr;
            if (scene) {
                scene->onRemove({}, child);
            }
            return;
        }
        ++it;
    }
}

void Entity::removeChildByIndex(uint32_t index) {
    assert(index < children.size());
    auto entity = children[index];
    entity->parent = nullptr;
    children.erase(children.begin() + index);
    if (scene) {
        scene->onRemove({}, entity);
    }
}


void Entity::invalidate(EntityInvalidateType type) {
    if (scene) {
        Internal::EntityUpdateType updateType;
        switch (type) {
            case EntityInvalidateType::Transform:
                updateType = Internal::EntityUpdateType::Transform;
                break;
            default:
                updateType = Internal::EntityUpdateType::Other;
                break;
        }
        scene->onInvalidate({}, this, static_cast<int>(updateType));
    }
}

void Entity::invalidateComponentAdd() {
    if (scene) {
        scene->onInvalidate({}, this, static_cast<int>(Internal::EntityUpdateType::ComponentAdd));
    }
}

void Entity::invalidateComponentRemove() {
    if (scene) {
        scene->onInvalidate({}, this, static_cast<int>(Internal::EntityUpdateType::ComponentRemove));
    }
}


}