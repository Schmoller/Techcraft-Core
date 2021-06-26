#pragma once

#include "common.hpp"
#include "../forward.hpp"
#include <memory>

namespace Engine {

class Texture {
public:
    Texture(std::string name, std::shared_ptr<Image> image, std::shared_ptr<Internal::SamplerRef> sampler);

    const std::string &getName() const { return name; }

    uint32_t getWidth() const;
    uint32_t getHeight() const;

    const std::shared_ptr<Image> &getImage() const { return image; }

    const std::shared_ptr<Internal::SamplerRef> &getSampler() const { return sampler; }
private:
    const std::string name;
    std::shared_ptr<Image> image;
    std::shared_ptr<Internal::SamplerRef> sampler;
};

typedef std::shared_ptr<Texture> SharedTexture;

}