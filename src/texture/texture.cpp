#include "tech-core/texture/texture.hpp"
#include "tech-core/image.hpp"

namespace Engine {

Texture::Texture(std::string name, std::shared_ptr<Image> image, std::shared_ptr<Internal::SamplerRef> sampler)
    : name(std::move(name)), image(std::move(image)), sampler(std::move(sampler)) {
}

uint32_t Texture::getWidth() const {
    return image->getWidth();
}

uint32_t Texture::getHeight() const {
    return image->getHeight();
}


}