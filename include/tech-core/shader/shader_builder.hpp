#pragma once

#include "../forward.hpp"
#include <memory>

namespace Engine {

class ShaderBuilder {
    friend class Shader;
public:
    ShaderBuilder &withStage(std::shared_ptr<ShaderStage>);

    std::shared_ptr<Shader> build() const;

private:
    std::shared_ptr<ShaderStage> vertexStage;
    std::shared_ptr<ShaderStage> fragmentStage;
};

}