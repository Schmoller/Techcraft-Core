#pragma once

#include "tech-core/forward.hpp"
#include <concepts>

namespace Engine {

class Component {
public:
    explicit Component(Entity &owner)
        : owner(owner) {};

    virtual ~Component() = default;
protected:
    Entity &owner;
};

template<typename T>
concept IsComponent = std::is_base_of<Component, T>::value;

}