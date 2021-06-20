#pragma once

#include <concepts>

namespace Engine {

class Component {
public:
    virtual ~Component() = default;
};

template<typename T>
concept IsComponent = std::is_base_of<Component, T>::value;

}