#include "tech-core/texture/builder.hpp"
#include "tech-core/texture/manager.hpp"
#include <stb_image.h>
#include <stdexcept>

namespace Engine {


TextureBuilder::TextureBuilder(TextureManager &manager, std::string name)
    : manager(manager),
    name(std::move(name)),
    pixelData(nullptr),
    width(0),
    height(0),
    mipType(MipType::NoMipmap),
    sourcedFromFile(false) {}

TextureBuilder &TextureBuilder::fromFile(const std::string &filename) {
    int texWidth, texHeight, texChannels;

    stbi_uc *pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw TextureLoadError(filename);
    }

    if (this->pixelData && this->sourcedFromFile) {
        stbi_image_free(this->pixelData);
    }

    this->pixelData = pixels;
    this->width = texWidth;
    this->height = texHeight;
    this->sourcedFromFile = true;

    return *this;
}

TextureBuilder &TextureBuilder::fromRaw(uint32_t width, uint32_t height, uint32_t *pixels) {
    if (this->pixelData && this->sourcedFromFile) {
        stbi_image_free(this->pixelData);
    }

    this->pixelData = pixels;
    this->width = width;
    this->height = height;
    this->sourcedFromFile = false;

    return *this;
}

TextureBuilder &TextureBuilder::withMipMode(MipType type) {
    mipType = type;

    return *this;
}

const Texture *TextureBuilder::build() {
    if (!pixelData || width == 0 || height == 0) {
        throw std::runtime_error("Incomplete texture definition");
    }

    auto texture = manager.addTexture(name, width, height, pixelData, mipType);

    if (sourcedFromFile) {
        stbi_image_free(pixelData);
    }

    return texture;
}

}