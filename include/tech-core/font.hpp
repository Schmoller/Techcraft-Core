#pragma once

#include "forward.hpp"
#include <stb_truetype.h>
#include "tech-core/gui/common.hpp"

#include <array>

namespace Engine {

enum class FontStyle {
    Regular,
    Italic,
    Bold,
    BoldItalic
};

enum class Alignment {
    Begining,
    Middle,
    End
};

const std::array<std::array<uint16_t, 2>, 7> CODE_POINT_RANGES = {{
    { 0x0020, 0x007E }, // Ascii
    { 0x00A7, 0x00A7 }, // Section symbol
    { 0x00B0, 0x00B0 }, // Degree
    { 0x00B5, 0x00B5 }, // Micro
    { 0x2190, 0x2199 }, // Arrows
    { 0x2500, 0x257F }, // Box drawing
    { 0x2580, 0x259F }, // Block chars
}};

#define FONT_ATLAS_SIZE 512
#define FONT_OVERSAMPLING 1
// Spacing between chars
#define FONT_PADDING 1

class Font {
    friend class FontManager;
public:
    void draw(
        const std::wstring &text,
        Gui::Drawer &drawer,
        const glm::vec2 &offset = {},
        Alignment hAlign = Alignment::Begining,
        Alignment vAlign = Alignment::Begining,
        uint32_t color = 0xFFFFFFFF,
        glm::vec2 *outCoords = nullptr
    );

    float getFontSize() {
        return fontSize;
    }

    float getLineHeight() {
        return ascent - descent + lineGap;
    }

    void computeSize(const std::wstring &text, float &outWidth, float &outHeight);

private:
    Font(
        FontStyle style,
        int oversampling,
        const Texture *texture,
        float fontSize,
        float ascent,
        float descent,
        float lineGap
    );

    FontStyle style;
    int oversampling;
    float fontSize;
    const Texture *texture;
    std::unordered_map<wchar_t, stbtt_packedchar> codePoints;

    // Positional information
    float ascent;
    float descent;
    float lineGap;
};

class FontManager {
    typedef std::unordered_multimap<std::string, Font> FontMap;

public:
    explicit FontManager(TextureManager &textureManager);

    Font *addFont(const std::string &filename, const std::string &name, FontStyle style, float fontSize = 20);
    Font *getFont(const std::string &name, FontStyle style);

    const Texture *getFontTexture(const std::string &name, FontStyle style);

private:
    TextureManager &textureManager;
    FontMap supportedFonts;
};

}