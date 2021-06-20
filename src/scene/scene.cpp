#include "tech-core/scene/scene.hpp"
#include "tech-core/scene/entity.hpp"
#include <cassert>

namespace Engine {

void Scene::addChild(const std::shared_ptr<Entity> &entity) {
    children.push_back(entity);
    childrenById[entity->getId()] = entity;
    entity->setScene({}, this);
}

std::shared_ptr<Entity> Scene::getChildById(EntityId id) const {
    auto it = childrenById.find(id);
    if (it != childrenById.end()) {
        return it->second;
    }

    return {};
}

std::shared_ptr<Entity> Scene::getChildByIndex(uint32_t index) const {
    assert(index < children.size());
    return children[index];
}

void Scene::removeChildById(EntityId id) {
    auto it = childrenById.find(id);
    if (it != childrenById.end()) {
        auto &child = it->second;
        if (child->getParent() == nullptr) {
            // One of our children
            auto childIt = children.begin();
            while (childIt != children.end()) {
                auto &other = *childIt;
                if (other->getId() == id) {
                    children.erase(childIt);
                    return;
                }
            }
            childrenById.erase(it);
        } else {
            child->getParent()->removeChildById(id);
        }
    }
}

void Scene::removeChildByIndex(uint32_t index) {
    assert(index < children.size());
    children.erase(children.begin() + index);
}

void Scene::onAdd(Badge<Entity>, const std::shared_ptr<Entity> &entity) {
    childrenById[entity->getId()] = entity;
}

void Scene::onRemove(Badge<Entity>, const std::shared_ptr<Entity> &entity) {
    auto it = childrenById.find(entity->getId());
    if (it != childrenById.end()) {
        childrenById.erase(it);
    }
}

void Scene::onInvalidate(Badge<Entity>, Entity *entity) {

}


}