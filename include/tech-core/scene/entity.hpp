#pragma once

#include "components/base.hpp"
#include "../forward.hpp"
#include "../types.hpp"
#include "scene.hpp"
#include "components/transform.hpp"

#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>

namespace Engine {

class Entity final {
public:
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

    void addChild(const std::shared_ptr<Entity> &entity);
    void removeChildById(EntityId);
    void removeChildByIndex(uint32_t);

    void invalidate();
private:
    const EntityId id;
    Entity *parent { nullptr };
    Scene *scene { nullptr };
    std::vector<std::shared_ptr<Entity>> children;

    Transform transform;
    std::unordered_map<size_t, Component *> components;

    Entity(EntityId, std::unordered_map<size_t, Component *> components);
};

template<IsComponent T>
T &Entity::get() {
    auto it = components.find(typeid(T).hash_code());
    assert(it != components.end());
    return *reinterpret_cast<T *>(it->second);
}

template<IsComponent T>
const T &Entity::get() const {
    auto it = components.find(typeid(T).hash_code());
    assert(it != components.end());
    return *reinterpret_cast<T *>(it->second);
}

template<IsComponent T>
bool Entity::has() {
    auto it = components.find(typeid(T).hash_code());
    return it != components.end();
}

template<IsComponent T, typename... Args>
T &Entity::add(Args... args) {
    auto it = components.emplace(typeid(T).hash_code(), { args... });
    assert(it.second);
    invalidate();
    return it.first.second;
}

template<IsComponent T>
void Entity::remove() {
    auto it = components.find(typeid(T).hash_code());
    assert(it != components.end());
    components.erase(it);
}

namespace Internal {

/**
 * Allocates memory for each of the provided components.
 */
template<IsComponent First, IsComponent... Rest>
void entityInitComponents(std::unordered_map<size_t, unsigned char *> &memory) {
    memory[typeid(First).hash_code()] = new First();

    if constexpr(sizeof...(Rest) > 0) {
        entityInitComponents<Rest...>(memory);
    }
}

}

template<IsComponent... Types>
std::shared_ptr<Entity> Entity::createEntity(EntityId id) {
    std::unordered_map<size_t, unsigned char *> memory;
    memory.reserve(sizeof...(Types));
    Internal::entityInitComponents<Types...>(memory);

    return std::make_shared<Entity>(id, memory);
}

}