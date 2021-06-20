#include "tech-core/scene/entity.hpp"

namespace Engine {

Entity::Entity(EntityId id, std::unordered_map<size_t, Component *> components)
    : id(id),
    components(std::move(components)),
    transform(*this) {

}

Entity::~Entity() {
    for (auto &pair : components) {
        delete pair.second;
    }
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

void Entity::addChild(const std::shared_ptr<Entity> &entity) {
    assert(!entity->getParent());

    entity->parent = this;
    children.push_back(entity);
    if (scene) {
        entity->scene = scene;
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
            child->scene = nullptr;
            if (scene) {
                scene->onRemove({}, child);
            }
            return;
        }
    }
}

void Entity::removeChildByIndex(uint32_t index) {
    assert(index < children.size());
    auto entity = children[index];
    entity->parent = nullptr;
    entity->scene = nullptr;
    children.erase(children.begin() + index);
    if (scene) {
        scene->onRemove({}, entity);
    }
}

void Entity::invalidate() {
    if (scene) {
        scene->onInvalidate({}, this);
    }
}

}