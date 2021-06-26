#pragma once

#include "common.hpp"
#include "tech-core/mesh.hpp"
#include "tech-core/font.hpp"

#include <vector>
#include <unordered_map>

namespace Engine::Gui {

const uint32_t DEFAULT_GUI_TEXT_PALLET[10] = {
    0x000000FF, // Black
    0x808080FF, // Dark Grey
    0xC0C0C0FF, // Light Grey
    0x0087FFFF, // Blue
    0x00D700FF, // Green
    0xD70000FF, // Red
    0xD7FF00FF, // Yellow
    0xFF00D7FF, // Magenta
    0x00D7AFFF, // Cyan
    0xFFFFFFFF  // White
};

const uint32_t DEFAULT_GUI_TEXT_COLOR = 0xFFFFFFFF;

const wchar_t TEXT_ESCAPE_CHAR = L'\033';

enum class StrokePosition {
    Inside,
    Center,
    Outside
};

class Drawer {
    friend class GuiManager;
    struct Region {
        const Texture *texture;
        std::vector<Vertex> vertices;
        std::vector<GuiBufferInt> indices;
    };

public:
    Drawer(FontManager &fontManager, const Texture &whiteTexture, const std::string &defaultFontName);

    void drawRect(const Rect &rect, const Texture &texture);
    void drawRect(const Rect &rect, const Texture &texture, const Rect &sourceRect, uint32_t color = 0xFFFFFFFF);
    void drawRect(const Rect &rect, uint32_t colour);

    void drawRectOutline(const Rect &rect, uint32_t strokeSize, StrokePosition strokePos, uint32_t strokeColour);

    void drawLine(const glm::vec2 &from, const glm::vec2 &to, uint32_t colour, uint32_t strokeSize = 1);

    void drawText(
        const std::string &text, float x, float y, Font *font, Alignment hAlign = Alignment::Begining,
        Alignment vAlign = Alignment::Begining, uint32_t colour = DEFAULT_GUI_TEXT_COLOR
    );
    void drawText(
        const std::wstring &text, float x, float y, Font *font, Alignment hAlign = Alignment::Begining,
        Alignment vAlign = Alignment::Begining, uint32_t colour = DEFAULT_GUI_TEXT_COLOR
    );

    void drawTextWithFormatting(const std::string &text, float x, float y, uint32_t color = DEFAULT_GUI_TEXT_COLOR);
    void drawTextWithFormatting(const std::wstring &text, float x, float y, uint32_t color = DEFAULT_GUI_TEXT_COLOR);

    void draw(const std::vector<Vertex> &vertices, const std::vector<GuiBufferInt> &indices, const Texture &texture);


    void translate(float x, float y);
    void rotate(float angle);
    void scale(float x, float y);
    void scale(float all);

    void resetTransform();
    void pushTransform(bool reset = false);
    void popTransform();

    FontManager &getFontManager() const { return fontManager; }


protected:
    void reset();

private:
    Vertex transformVertex(const Vertex &vertex);

    const std::string defaultFontName;

    FontManager &fontManager;
    const Texture &whiteTexture;
    Region *currentRegion = nullptr;
    std::vector<Region> regions;

    glm::mat4 transform { 1.0f };
    std::vector<glm::mat4> transformStack;
};

}