#pragma once

#include <string>
#include <exception>

// Forward def
typedef void *ImTextureID;

namespace Engine {

struct Texture {
    uint32_t arrayId { 0 };
    size_t arraySlot { 0 };
    bool mipmaps { false };
    uint32_t width { 0 };
    uint32_t height { 0 };

    ImTextureID toImGui() const {
        return reinterpret_cast<ImTextureID>(
            const_cast<Texture *>(this)
        );
    }

    explicit operator ImTextureID() const {
        return reinterpret_cast<ImTextureID>(
            const_cast<Texture *>(this)
        );
    }
};

enum MipType {
    NoMipmap = 0,
    StoredStandard,
    Generate
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

}