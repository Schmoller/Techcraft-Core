#include "tech-core/font.hpp"
#include "tech-core/gui/drawer.hpp"
#include "tech-core/texture/manager.hpp"
#include "tech-core/texture/builder.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION

#include <stb_rect_pack.h>
#include <stb_truetype.h>

#include <fstream>
#include <vector>
#include <sstream>

namespace Engine {

// Forward declarations
std::string toString(FontStyle style);

Font::Font(
    FontStyle style,
    int oversampling,
    const Texture *texture,
    float fontSize,
    float ascent,
    float descent,
    float lineGap
) : style(style),
    oversampling(oversampling),
    texture(texture),
    fontSize(fontSize),
    ascent(ascent),
    descent(descent),
    lineGap(lineGap) {}

void Font::computeSize(const std::wstring &text, float &outWidth, float &outHeight) {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;

    stbtt_aligned_quad quad;

    for (auto chr : text) {
        if (chr == L'\n') {
            x = 0;
            y += ascent - descent + lineGap;
        }

        auto result = codePoints.find(chr);
        if (result == codePoints.end()) {
            // Dont render unknown chars
            continue;
        }

        stbtt_GetPackedQuad(
            &result->second,
            oversampling,
            oversampling,
            0,
            &x,
            &y,
            &quad,
            0
        );

        width = std::max(width, x);
    }

    height = y + ascent - descent;

    outWidth = width;
    outHeight = height;
}

void Font::draw(
    const std::wstring &text,
    Gui::Drawer &drawer,
    const glm::vec2 &offset,
    Alignment hAlign,
    Alignment vAlign,
    uint32_t color,
    glm::vec2 *outCoords
) {
    // Get overall size
    float width, height;
    computeSize(text, width, height);

    stbtt_aligned_quad quad;

    float y;
    float maxX = 0;
    // TODO: Allow rendering as Y = baseline
    switch (vAlign) {
        case Alignment::Begining:
            y = 0;
            break;
        case Alignment::Middle:
            y = -height / 2.0f;
            break;
        case Alignment::End:
            y = -height;
            break;
    }

    std::wstringstream stream(text);
    std::wstring line;
    bool first = true;
    while (std::getline(stream, line, L'\n')) {
        if (!first) {
            y += ascent - descent + lineGap;
        }
        first = false;

        float lineWidth, lineHeight;
        if (hAlign != Alignment::Begining || vAlign != Alignment::Begining) {
            computeSize(line, lineWidth, lineHeight);
        } else {
            lineWidth = 0;
            lineHeight = 0;
        }

        float x;

        switch (hAlign) {
            case Alignment::Begining:
                x = 0;
                break;
            case Alignment::Middle:
                x = -lineWidth / 2.0f;
                break;
            case Alignment::End:
                x = -lineWidth;
                break;
        }

        for (auto chr : line) {
            auto result = codePoints.find(chr);
            if (result == codePoints.end()) {
                // Dont render unknown chars
                continue;
            }

            stbtt_GetPackedQuad(
                &result->second,
                oversampling,
                oversampling,
                0,
                &x,
                &y,
                &quad,
                0
            );

            // Output quad
            // TODO: Allow rendering from baseline (just remove the + ascent parts)
            drawer.drawRect(
                Gui::Rect {{ quad.x0 + offset.x, quad.y0 + offset.y + ascent },
                    { quad.x1 + offset.x, quad.y1 + offset.y + ascent }},
                *texture,
                Gui::Rect {
                    { quad.s0, quad.t0 },
                    { quad.s1, quad.t1 }
                },
                color
            );
        }

        maxX = std::max(maxX, x);
    }

    if (outCoords) {
        *outCoords = { maxX, y };
    }
}


FontManager::FontManager(TextureManager &textureManager)
    : textureManager(textureManager) {}

Font *FontManager::addFont(const std::string &filename, const std::string &name, FontStyle style, float fontSize) {
    std::ifstream input(filename);

    if (!input.is_open()) {
        throw std::runtime_error("Failed to open font");
    }

    input.seekg(0, input.end);
    auto length = input.tellg();
    input.seekg(0, input.beg);

    unsigned char *buffer = new unsigned char[length];

    input.read(reinterpret_cast<char *>(buffer), length);
    if (input.fail()) {
        delete[] buffer;
        throw std::runtime_error("Failed to read font");
    }

    // Load up font
    auto fontCount = stbtt_GetNumberOfFonts(buffer);

    if (fontCount == 1) {
        // Not supporing multiple fonts per file
        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, buffer, 0)) {
            delete[] buffer;
            throw std::runtime_error("Font file is invalid");
        }

        // Compute font size info
        int rawAscent, rawDescent, rawLineGap;
        stbtt_GetFontVMetrics(&info, &rawAscent, &rawDescent, &rawLineGap);

        float fontScale = fontSize / (rawAscent - rawDescent);

        float ascent = rawAscent * fontScale;
        float descent = rawDescent * fontScale;
        float lineGap = rawLineGap * fontScale;

        stbtt_pack_context context;
        uint8_t *pixelData = new uint8_t[FONT_ATLAS_SIZE * FONT_ATLAS_SIZE];

        // TODO: Check return codes
        stbtt_PackBegin(&context, pixelData, FONT_ATLAS_SIZE, FONT_ATLAS_SIZE, 0, FONT_PADDING, nullptr);

        std::vector<stbtt_pack_range> ranges(CODE_POINT_RANGES.size());
        std::vector<std::vector<stbtt_packedchar>> outputs(CODE_POINT_RANGES.size());

        for (size_t i = 0; i < CODE_POINT_RANGES.size(); ++i) {
            auto range = CODE_POINT_RANGES[i];

            int count = range[1] - range[0] + 1;

            outputs[i].resize(count);
            ranges[i] = {
                fontSize,
                range[0],
                nullptr,
                count, // Inclusive range
                outputs[i].data()
            };

        }

        stbtt_PackSetOversampling(&context, FONT_OVERSAMPLING, FONT_OVERSAMPLING);
        stbtt_PackFontRanges(&context, buffer, 0, ranges.data(), ranges.size());

        stbtt_PackEnd(&context);

        // Convert to R8G8B8A8 format
        uint32_t *convertedPixelData = new uint32_t[FONT_ATLAS_SIZE * FONT_ATLAS_SIZE];
        for (int i = 0; i < FONT_ATLAS_SIZE * FONT_ATLAS_SIZE; ++i) {
            convertedPixelData[i] = (
                0x00FFFFFF | pixelData[i] << 24
            );
        }

        std::stringstream textureName;
        textureName << "font." << name << "." << toString(style);

        auto *texture = textureManager.createTexture(textureName.str())
            .fromRaw(FONT_ATLAS_SIZE, FONT_ATLAS_SIZE, convertedPixelData)
            .build();

        // Prepare for use
        Font fontDefinition(style, FONT_OVERSAMPLING, texture, fontSize, ascent, descent, lineGap);

        for (size_t i = 0; i < CODE_POINT_RANGES.size(); ++i) {
            auto cpRange = CODE_POINT_RANGES[i];

            for (uint16_t codePoint = cpRange[0], offset = 0; codePoint <= cpRange[1]; ++codePoint, ++offset) {
                fontDefinition.codePoints[codePoint] = outputs[i][offset];
            }
        }

        supportedFonts.insert(std::pair(name, fontDefinition));

        delete[] buffer;
        delete[] pixelData;
        delete[] convertedPixelData;
    } else {
        delete[] buffer;
        throw std::runtime_error("Font file contains multiple fonts. This is unsuported");
    }

    return getFont(name, style);
}

Font *FontManager::getFont(const std::string &fontName, FontStyle style) {
    // Find the details for the requested font.
    // Fall back to the regular font style if requested style is not found.
    FontMap::iterator it, end;
    std::tie(it, end) = supportedFonts.equal_range(fontName);

    Font *best = nullptr;
    for (; it != end; ++it) {
        if (it->second.style == style) {
            best = &it->second;
            break;
        } else if (it->second.style == FontStyle::Regular) {
            best = &it->second;
        }
    }

    return best;
}

const Texture *FontManager::getFontTexture(const std::string &name, FontStyle style) {
    std::stringstream textureName;
    textureName << name << "." << toString(style);

    return textureManager.getTexture(textureName.str());
}

std::string toString(FontStyle style) {
    switch (style) {
        case FontStyle::Bold:
            return "bold";
        case FontStyle::Regular:
            return "regular";
        case FontStyle::Italic:
            return "italic";
        case FontStyle::BoldItalic:
            return "bolditalic";
    };
    return "Unknown";
}

}
