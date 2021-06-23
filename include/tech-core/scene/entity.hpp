#pragma once

#include "components/base.hpp"
#include "../forward.hpp"
#include "../types.hpp"
#include "scene.hpp"
#include "components/transform.hpp"

#include <memory>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <functional>

namespace Engine {

enum class EntityInvalidateType {
    Transform,
    Render
};

class Entity final {
public:
    explicit Entity(EntityId);

    template<IsComponent ...Types>
    static std::shared_ptr<Entity> createEntity(EntityId);

    ~Entity();

    EntityId getId() const { return id; }

    Transform &getTransform() { return transform; }

    const Transform &getTransform() const { return transform; }

    template<IsComponent T>
    bool has();

    template<IsComponent T>
    T &get();

    template<IsComponent T>
    const T &get() const;

    template<IsComponent T, typename... Args>
    T &add(Args ...args);

    template<IsComponent T>
    void remove();

    Entity *getParent() const { return parent; };

    Scene *getScene() const { return scene; }

    void setScene(Badge<Scene>, Scene *);

    const std::vector<std::shared_ptr<Entity>> &getChildren() const { return children; }

    std::shared_ptr<Entity> getChildById(EntityId) const;
    std::shared_ptr<Entity> getChildByIndex(uint32_t) const;
    void forEachChild(bool allDescendants, const std::function<void(Entity *)> &callback) const;

    void addChild(const std::shared_ptr<Entity> &entity);
    void removeChildById(EntityId);
    void removeChildByIndex(uint32_t);

    void invalidate(EntityInvalidateType);
private:
    const EntityId id;
    Entity *parent { nullptr };
    Scene *scene { nullptr };
    std::vector<std::shared_ptr<Entity>> children;

    Transform transform;
    std::unordered_map<size_t, Component> components;

    void invalidateComponentAdd();
    void invalidateComponentRemove();
};

template<IsComponent T>
T &Entity::get() {
    auto it = components.find(typeid(T).hash_code());
    assert(it != components.end());
    return static_cast<T &>(it->second);
}

template<IsComponent T>
const T &Entity::get() const {
    auto it = components.find(typeid(T).hash_code());
    assert(it != components.end());
    return static_cast<T &>(it->second);
}

template<IsComponent T>
bool Entity::has() {
    auto it = components.find(typeid(T).hash_code());
    return it != components.end();
}

template<IsComponent T, typename... Args>
T &Entity::add(Args... args) {
    auto it = components.emplace(typeid(T).hash_code(), T { *this, args... });
    assert(it.second);
    invalidateComponentAdd();
    return static_cast<T &>(it.first->second);
}

template<IsComponent T>
void Entity::remove() {
    auto it = components.find(typeid(T).hash_code());
    assert(it != components.end());
    invalidateComponentRemove();
    components.erase(it);
}

namespace Internal {

/**
 * Allocates memory for each of the provided components.
 */
template<IsComponent First, IsComponent... Rest>
void entityInitComponents(std::shared_ptr<Entity> &entity) {
    entity->add<First>();

    if constexpr(sizeof...(Rest) > 0) {
        entityInitComponents<Rest...>(entity);
    }
}

}

template<IsComponent... Types>
std::shared_ptr<Entity> Entity::createEntity(EntityId id) {
    auto entity = std::make_shared<Entity>(id);
    Internal::entityInitComponents<Types...>(entity);

    return entity;
}

}
