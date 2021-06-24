#include "tech-core/scene/scene.hpp"
#include "tech-core/scene/entity.hpp"
#include "render_planner.hpp"
#include <cassert>

namespace Engine {

void Scene::addChild(const std::shared_ptr<Entity> &entity) {
    children.push_back(entity);
    childrenById[entity->getId()] = entity;
    entity->setScene({}, this);

    if (renderPlanner) {
        entity->forEachChild(
            true,
            [this](Entity *child) {
                renderPlanner->prepareEntity(child);
            }
        );

        renderPlanner->addEntity(entity.get());
    }

    entity->forEachChild(
        true,
        [this](Entity *child) {
            child->setScene({}, this);
            if (renderPlanner) {
                renderPlanner->addEntity(child);
            }
        }
    );
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
        auto childIt = children.begin();
        while (childIt != children.end()) {
            auto &other = *childIt;
            if (other->getId() == id) {
                children.erase(childIt);
                break;
            }
        }
        childrenById.erase(it);
        child->setScene({}, nullptr);
        if (renderPlanner) {
            renderPlanner->removeEntity(child.get());
        }
    }
}

void Scene::removeChildByIndex(uint32_t index) {
    assert(index < children.size());
    auto &entity = children[index];
    children.erase(children.begin() + index);
    entity->setScene({}, nullptr);

    auto it = childrenById.find(entity->getId());
    if (it != childrenById.end()) {
        childrenById.erase(it);
    }
    if (renderPlanner) {
        renderPlanner->removeEntity(entity.get());
    }
}

void Scene::onAdd(Badge<Entity>, const std::shared_ptr<Entity> &entity) {
    entity->setScene({}, this);
    if (renderPlanner) {
        entity->forEachChild(
            true,
            [this](Entity *child) {
                renderPlanner->prepareEntity(child);
            }
        );

        renderPlanner->addEntity(entity.get());
    }
    entity->forEachChild(
        true,
        [this](Entity *child) {
            child->setScene({}, this);
            if (renderPlanner) {
                renderPlanner->addEntity(child);
            }
        }
    );
}

void Scene::onRemove(Badge<Entity>, const std::shared_ptr<Entity> &entity) {
    entity->setScene({}, nullptr);
    if (renderPlanner) {
        renderPlanner->removeEntity(entity.get());
    }
    entity->forEachChild(
        true,
        [this](Entity *child) {
            child->setScene({}, nullptr);
            if (renderPlanner) {
                renderPlanner->removeEntity(child);
            }
        }
    );
}

void Scene::onInvalidate(Badge<Entity>, Entity *entity, int type) {
    if (renderPlanner) {
        renderPlanner->updateEntity(entity, static_cast<Internal::EntityUpdateType>(type));
    }
}

void Scene::onSetActive(Badge<RenderEngine>, Internal::RenderPlanner *planner) {
    renderPlanner = planner;
    for (auto &child : children) {
        // This must be done first to ensure that all entities have PlannerData
        child->forEachChild(
            true, [this](Entity *grandChild) {
                renderPlanner->prepareEntity(grandChild);
            }
        );

        // Now we can add
        renderPlanner->addEntity(child.get());
        child->forEachChild(
            true, [this](Entity *grandChild) {
                renderPlanner->addEntity(grandChild);
            }
        );
    }
}

void Scene::onSetInactive(Badge<RenderEngine>) {
    for (auto &child : children) {
        renderPlanner->removeEntity(child.get());
        child->forEachChild(
            true, [this](Entity *grandChild) {
                renderPlanner->removeEntity(grandChild);
            }
        );
    }
    renderPlanner = nullptr;
}


}