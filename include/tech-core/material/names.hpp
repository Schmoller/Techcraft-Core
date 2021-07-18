#pragma once

#include <string>

namespace Engine {

struct MaterialVariables {
    static const std::string_view AlbedoTexture;
    static const std::string_view NormalTexture;
    static const std::string_view RoughnessTexture;
    static const std::string_view OcclusionTexture;
    static const std::string_view MetalnessTexture;
};

}