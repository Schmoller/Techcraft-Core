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

    TextureBuilder &withMipMaps(TextureMipType);

    TextureBuilder &withWrapMode(TextureWrapMode);
    TextureBuilder &withWrapModeU(TextureWrapMode);
    TextureBuilder &withWrapModeV(TextureWrapMode);
    TextureBuilder &withFiltering(TextureFilterMode);
    /**
     * The maximum level of anisotropy to sample this texture with.
     * The actual level depends on the capabilities of the GPU and the current engine settings
     */
    TextureBuilder &withAnisotropy(float);

    const Texture *finish();
private:
    TextureBuilder(TextureManager &manager, std::string name);

    // Non-configurable
    TextureManager &manager;
    std::string name;

    // Configurable
    void *pixelData { nullptr };
    bool sourcedFromFile { false };
    uint32_t width { 0 };
    uint32_t height { 0 };
    TextureMipType mipType { TextureMipType::None };
    TextureWrapMode wrapU { TextureWrapMode::Repeat };
    TextureWrapMode wrapV { TextureWrapMode::Repeat };
    TextureFilterMode filtering { TextureFilterMode::Linear };
    float anisotropy { 0 };
};

}