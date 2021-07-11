#pragma once

#include "../forward.hpp"
#include <memory>

namespace Engine {

class ShaderBuilder {
    friend class Shader;
public:
    explicit ShaderBuilder(std::string name);

    ShaderBuilder &withStage(std::shared_ptr<ShaderStage>);

    std::shared_ptr<Shader> build() const;

private:
    std::string name;

    std::shared_ptr<ShaderStage> vertexStage;
    std::shared_ptr<ShaderStage> fragmentStage;
};

}