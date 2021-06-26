#pragma once

#include <string>
#include <exception>

namespace Engine {

enum class TextureMipType {
    None,
    StoredStandard,
    Generate
};

enum class TextureWrapMode {
    Repeat,
    Mirror,
    Clamp,
    Border
};

enum class TextureFilterMode {
    None,
    Linear,
    Cubic
};

class TextureLoadError : public std::exception {
public:
    explicit TextureLoadError(std::string filename)
        : filename(std::move(filename)) {}

    virtual const char *what() const noexcept override {
        return filename.c_str();
    }

private:
    std::string filename;
};

namespace Internal {
class SamplerRef;
class SamplerCache;
struct SamplerSettings;
}

}