#include "imageutils.hpp"

#define BLANK_PIXEL 0x00000000;
#define BLACK_PIXEL 0xFF000000;
#define MAGENTA_PIXEL 0xFFFF00FF;

namespace Engine {

void generateErrorPixels(uint32_t width, uint32_t height, uint32_t *pixels) {
    for (uint32_t x = 0; x < width; ++x) {
        for (uint32_t y = 0; y < height; ++y) {
            if (x > width/2) {
                if (y > height/2) {
                    pixels[x + y * width] = MAGENTA_PIXEL;
                } else {
                    pixels[x + y * width] = BLACK_PIXEL;
                }
            } else {
                if (y > height/2) {
                    pixels[x + y * width] = BLACK_PIXEL;
                } else {
                    pixels[x + y * width] = MAGENTA_PIXEL;
                }
            }
        }
    }
}

void generateSolidPixels(uint32_t width, uint32_t height, uint32_t *pixels, uint32_t colour) {
    for (uint32_t i = 0; i < width * height; ++i) {
        pixels[i] = colour;
    }
}

}