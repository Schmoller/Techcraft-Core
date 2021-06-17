#pragma once

#include "tech-core/texture/common.hpp"
#include "tech-core/forward.hpp"
#include <string>

namespace Engine {

class TextureBuilder {
    friend class TextureManager;
public:
    /**
     * Sources the pixel data from a texture on the filesystem.
     */
    TextureBuilder &fromFile(const std::string &filename);
    /**
     * Sources the pixel data from raw data.
     * The pixel format is expected to be RGBA 8 bits per pixel.
     * The pixels array must have width * height elements.
     */
    TextureBuilder &fromRaw(uint32_t width, uint32_t height, uint32_t *pixels);
    /**
     * Changes the mipmap mode.
     */
    TextureBuilder &withMipMode(MipType type);
    const Texture *build();
private:
    TextureBuilder(TextureManager &manager, std::string name);

    // Non-configurable
    TextureManager &manager;
    std::string name;

    // Configurable
    void *pixelData;
    bool sourcedFromFile;
    uint32_t width;
    uint32_t height;
    MipType mipType;
};

}