#ifndef IMAGEUTILS_HPP
#define IMAGEUTILS_HPP

#include <string>

// ================================
//  Placeholder image data
// ================================
#define PLACEHOLDER_TEXTURE_SIZE 128

namespace Engine {

void generateErrorPixels(uint32_t width, uint32_t height, uint32_t *pixels);
void generateSolidPixels(uint32_t width, uint32_t height, uint32_t *pixels, uint32_t colour);

}

#endif