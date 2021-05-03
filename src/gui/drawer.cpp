#include "tech-core/gui/drawer.hpp"

#include <locale>
#include <codecvt>
#include <sstream>

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> narrowToWideCharConverter;

#define STATE(name) case State::name:
#define SWITCH_STATE(name) \
    currentState = State::name; \
    continue

#define SWITCH_STATE_NO_CONSUME(name) \
    currentState = State::name; \
    --index; \
    continue

namespace Engine::Gui {

Drawer::Drawer(FontManager &fontManager, Texture &whiteTexture, const std::string &defaultFontName)
    : fontManager(fontManager), whiteTexture(whiteTexture), defaultFontName(defaultFontName)
{
    resetTransform();
}

void Drawer::drawRect(const Rect &rect, const Engine::Texture &texture) {
    // Region for this texture array
    // Create a new one every time the texture array switches to maintain vertex ordering
    Region *region;
    if (!currentRegion || currentRegion->textureArrayId != texture.arrayId) {
        region = &regions.emplace_back(Region{texture.arrayId});
        currentRegion = region;
    } else {
        region = currentRegion;
    }

    // Draw a rectangle
    GuiBufferInt startVertex = region->vertices.size();

    region->vertices.push_back(transformVertex({
        glm::vec3(rect.topLeft.x, rect.topLeft.y, 0),
        glm::vec3(0, 0, texture.arraySlot),
        glm::vec4(1, 1, 1, 1),
    }));
    region->vertices.push_back(transformVertex({
        glm::vec3(rect.bottomRight.x, rect.topLeft.y, 0),
        glm::vec3(1, 0, texture.arraySlot),
        glm::vec4(1, 1, 1, 1),
    }));
    region->vertices.push_back(transformVertex({
        glm::vec3(rect.bottomRight.x, rect.bottomRight.y, 0),
        glm::vec3(1, 1, texture.arraySlot),
        glm::vec4(1, 1, 1, 1),
    }));
    region->vertices.push_back(transformVertex({
        glm::vec3(rect.topLeft.x, rect.bottomRight.y, 0),
        glm::vec3(0, 1, texture.arraySlot),
        glm::vec4(1, 1, 1, 1),
    }));

    region->indices.push_back(startVertex + 0);
    region->indices.push_back(startVertex + 1);
    region->indices.push_back(startVertex + 2);

    region->indices.push_back(startVertex + 0);
    region->indices.push_back(startVertex + 2);
    region->indices.push_back(startVertex + 3);
}

void Drawer::drawRect(const Rect &rect, const Engine::Texture &texture, const Rect &sourceRect, uint32_t color) {
    // Region for this texture array
    // Create a new one every time the texture array switches to maintain vertex ordering
    Region *region;
    if (!currentRegion || currentRegion->textureArrayId != texture.arrayId) {
        region = &regions.emplace_back(Region{texture.arrayId});
        currentRegion = region;
    } else {
        region = currentRegion;
    }

    glm::vec4 colorVec = {
        static_cast<float>((color & 0xFF000000) >> 24) / 255.0f,
        static_cast<float>((color & 0x00FF0000) >> 16) / 255.0f,
        static_cast<float>((color & 0x0000FF00) >> 8)  / 255.0f,
        static_cast<float>((color & 0x000000FF) >> 0)  / 255.0f
    };

    // Draw a rectangle
    GuiBufferInt startVertex = region->vertices.size();

    region->vertices.push_back(transformVertex({
        glm::vec3(rect.topLeft.x, rect.topLeft.y, 0),
        glm::vec3(sourceRect.topLeft.x / texture.width, sourceRect.topLeft.y / texture.height, texture.arraySlot),
        colorVec
    }));
    region->vertices.push_back(transformVertex({
        glm::vec3(rect.bottomRight.x, rect.topLeft.y, 0),
        glm::vec3(sourceRect.bottomRight.x / texture.width, sourceRect.topLeft.y / texture.height, texture.arraySlot),
        colorVec
    }));
    region->vertices.push_back(transformVertex({
        glm::vec3(rect.bottomRight.x, rect.bottomRight.y, 0),
        glm::vec3(sourceRect.bottomRight.x / texture.width, sourceRect.bottomRight.y / texture.height, texture.arraySlot),
        colorVec
    }));
    region->vertices.push_back(transformVertex({
        glm::vec3(rect.topLeft.x, rect.bottomRight.y, 0),
        glm::vec3(sourceRect.topLeft.x / texture.width, sourceRect.bottomRight.y / texture.height, texture.arraySlot),
        colorVec
    }));

    region->indices.push_back(startVertex + 0);
    region->indices.push_back(startVertex + 1);
    region->indices.push_back(startVertex + 2);

    region->indices.push_back(startVertex + 0);
    region->indices.push_back(startVertex + 2);
    region->indices.push_back(startVertex + 3);
}

void Drawer::drawRect(const Rect &rect, uint32_t colour) {
    drawRect(rect, whiteTexture, {{0,0}, {1,1}}, colour);
}

void Drawer::drawRectOutline(const Rect &rect, uint32_t strokeSize, StrokePosition strokePos, uint32_t strokeColour) {
    glm::vec2 tlOuter, tlInner;
    glm::vec2 trOuter, trInner;
    glm::vec2 blOuter, blInner;
    glm::vec2 brOuter, brInner;

    switch (strokePos) {
        case StrokePosition::Center: {
            auto halfStroke = strokeSize / 2.0f;

            tlOuter = rect.topLeft - glm::vec2{halfStroke, halfStroke};
            tlInner = rect.topLeft + glm::vec2{halfStroke, halfStroke};

            trOuter = glm::vec2{rect.bottomRight.x + halfStroke, rect.topLeft.y - halfStroke};
            trInner = glm::vec2{rect.bottomRight.x - halfStroke, rect.topLeft.y + halfStroke};

            blOuter = glm::vec2{rect.topLeft.x - halfStroke, rect.bottomRight.y + halfStroke};
            blInner = glm::vec2{rect.topLeft.x + halfStroke, rect.bottomRight.y - halfStroke};

            brOuter = rect.bottomRight + glm::vec2{halfStroke, halfStroke};
            brInner = rect.bottomRight - glm::vec2{halfStroke, halfStroke};
            break;
        }
        case StrokePosition::Inside: {
            tlOuter = rect.topLeft;
            tlInner = rect.topLeft + glm::vec2{strokeSize, strokeSize};
            
            trOuter = {rect.bottomRight.x, rect.topLeft.y};
            trInner = {rect.bottomRight.x - strokeSize, rect.topLeft.y + strokeSize};
            
            blOuter = {rect.topLeft.x, rect.bottomRight.y};
            blInner = {rect.topLeft.x + strokeSize, rect.bottomRight.y - strokeSize};
            
            brOuter = rect.bottomRight;
            brInner = rect.bottomRight - glm::vec2{strokeSize, strokeSize};
            break;
        }
        case StrokePosition::Outside: {
            tlInner = rect.topLeft;
            tlOuter = rect.topLeft - glm::vec2{strokeSize, strokeSize};
            
            trInner = {rect.bottomRight.x, rect.topLeft.y};
            trOuter = {rect.bottomRight.x + strokeSize, rect.topLeft.y - strokeSize};
            
            blInner = {rect.topLeft.x, rect.bottomRight.y};
            blOuter = {rect.topLeft.x - strokeSize, rect.bottomRight.y + strokeSize};
            
            brInner = rect.bottomRight;
            brOuter = rect.bottomRight + glm::vec2{strokeSize, strokeSize};
            break;
        }
    }

    glm::vec4 colourVec = {
        static_cast<float>((strokeColour & 0xFF000000) >> 24) / 255.0f,
        static_cast<float>((strokeColour & 0x00FF0000) >> 16) / 255.0f,
        static_cast<float>((strokeColour & 0x0000FF00) >> 8)  / 255.0f,
        static_cast<float>((strokeColour & 0x000000FF) >> 0)  / 255.0f
    };
    
    std::vector<Vertex> vertices (8);
    std::vector<GuiBufferInt> indices (24);

    vertices[0] = {
        {tlOuter, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[1] = {
        {trOuter, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[2] = {
        {tlInner, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[3] = {
        {trInner, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[4] = {
        {blInner, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[5] = {
        {brInner, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[6] = {
        {blOuter, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[7] = {
        {brOuter, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };

    // Top
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 3;
    indices[3] = 3;
    indices[4] = 2;
    indices[5] = 0;

    // Right
    indices[6] = 1;
    indices[7] = 7;
    indices[8] = 5;
    indices[9] = 5;
    indices[10] = 3;
    indices[11] = 1;

    // Bottom
    indices[12] = 7;
    indices[13] = 6;
    indices[14] = 4;
    indices[15] = 4;
    indices[16] = 5;
    indices[17] = 7;

    // Left
    indices[18] = 6;
    indices[19] = 0;
    indices[20] = 2;
    indices[21] = 2;
    indices[22] = 4;
    indices[23] = 6;

    draw(vertices, indices, whiteTexture);
}

void Drawer::drawLine(const glm::vec2 &from, const glm::vec2 &to, uint32_t colour, uint32_t strokeSize) {
    glm::vec2 line = to - from;
    glm::vec2 lineDir = glm::normalize(line);
    glm::vec2 lineDirAdj = {lineDir.y, -lineDir.x};

    // Start and end vertices of polygon
    glm::vec2 startL = from - lineDirAdj * (strokeSize * 0.5f);
    glm::vec2 startR = from + lineDirAdj * (strokeSize * 0.5f);
    glm::vec2 endL = to - lineDirAdj * (strokeSize * 0.5f);
    glm::vec2 endR = to + lineDirAdj * (strokeSize * 0.5f);

    glm::vec4 colourVec = {
        static_cast<float>((colour & 0xFF000000) >> 24) / 255.0f,
        static_cast<float>((colour & 0x00FF0000) >> 16) / 255.0f,
        static_cast<float>((colour & 0x0000FF00) >> 8)  / 255.0f,
        static_cast<float>((colour & 0x000000FF) >> 0)  / 255.0f
    };

    std::vector<Vertex> vertices (4);
    std::vector<GuiBufferInt> indices (6);

    vertices[0] = {
        {startL, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[1] = {
        {endL, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[2] = {
        {endR, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };
    vertices[3] = {
        {startR, 0},
        {0,0, whiteTexture.arraySlot},
        colourVec
    };

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 0;

    draw(vertices, indices, whiteTexture);
}

void Drawer::drawText(const std::string &text, float x, float y, Font *font, Alignment hAlign, Alignment vAlign, uint32_t colour) {
    drawText(narrowToWideCharConverter.from_bytes(text), x, y, font, hAlign, vAlign, colour);
}

void Drawer::drawText(const std::wstring &text, float x, float y, Font *font, Alignment hAlign, Alignment vAlign, uint32_t colour) {
    font->draw(text, *this, {x, y}, hAlign, vAlign, colour);
}

void Drawer::drawTextWithFormatting(const std::string &text, float x, float y, uint32_t color) {
    drawTextWithFormatting(narrowToWideCharConverter.from_bytes(text), x, y, color);
}

void Drawer::drawTextWithFormatting(const std::wstring &text, float x, float y, uint32_t defaultColor) {
    Font *currentFont = fontManager.getFont(defaultFontName, FontStyle::Regular);
    std::string currentFontName = defaultFontName;

    uint32_t currentColor = defaultColor;
    bool bold = false;
    bool italic = false;
    float currentX = x;
    float currentY = y;

    std::wstringbuf buffer;

    enum class State {
        Text,
        EscapeStart,
        EscapeCode,
        EscapeArg,
        EscapeArgEnd
    };

    auto outputText = [&, this](const std::wstring &text) {
        glm::vec2 newCoords;
        currentFont->draw(
            text,
            *this,
            {currentX, currentY},
            Alignment::Begining,
            Alignment::Begining,
            currentColor,
            &newCoords
        );

        currentX += newCoords.x;
        currentY += newCoords.y;
    };

    auto updateFont = [&, this]() {
        FontStyle style;
        if (bold) {
            if (italic) {
                style = FontStyle::BoldItalic;
            } else {
                style = FontStyle::Bold;
            }
        } else {
            if (italic) {
                style = FontStyle::Italic;
            } else {
                style = FontStyle::Regular;
            }
        }

        currentFont = fontManager.getFont(currentFontName, style);
    };

    State currentState = State::Text;
    bool invertEscape = false;
    wchar_t escapeCode = 0;

    for (uint32_t index = 0; index <= text.size(); ++index) {
        wchar_t ch;
        if (index >= text.size()) {
            ch = 0;
        } else {
            ch = text[index];
        }

        switch (currentState) {
            STATE(Text) {
                switch (ch) {
                    case TEXT_ESCAPE_CHAR:
                        outputText(buffer.str());
                        buffer = std::wstringbuf{};
                        SWITCH_STATE(EscapeStart);
                        break;
                    case L'\n':
                        outputText(buffer.str());
                        buffer = std::wstringbuf{};
                        // TODO: Next line
                        currentX = x;
                        currentY += currentFont->getFontSize();
                        break;
                    case 0: // EOF
                        outputText(buffer.str());
                        buffer = std::wstringbuf{};
                        break;
                    default:
                        buffer.sputc(ch);
                        break;
                }
                break;
            }
            STATE(EscapeStart) {
                switch (ch) {
                    case L'!': // Invert action
                        invertEscape = true;
                        SWITCH_STATE(EscapeCode);
                        break;
                    default:
                        invertEscape = false;
                        SWITCH_STATE_NO_CONSUME(EscapeCode);
                        break;
                }
                break;
            }
            STATE(EscapeCode) {
                switch (ch) {
                    case L'0':
                    case L'1':
                    case L'2':
                    case L'3':
                    case L'4':
                    case L'5':
                    case L'6':
                    case L'7':
                    case L'8':
                    case L'9':
                        // Quick color code
                        currentColor = DEFAULT_GUI_TEXT_PALLET[ch - L'0'];

                        SWITCH_STATE(Text);
                        break;
                    case L'd': // Default color
                        currentColor = defaultColor;

                        SWITCH_STATE(Text);
                        break;
                    case L'b': // Bold
                        bold = !invertEscape;
                        updateFont();
                        SWITCH_STATE(Text);
                        break;
                    case L'i': // Italic
                        italic = !invertEscape;
                        updateFont();
                        SWITCH_STATE(Text);
                        break;
                    case L'r': // Reset
                        bold = false;
                        italic = false;
                        currentColor = defaultColor;
                        updateFont();
                        SWITCH_STATE(Text);
                        break;
                    case L'f': // Font
                    case L'c': // RGB(A) color
                        escapeCode = ch;
                        SWITCH_STATE(EscapeArg);
                        break;
                }
                break;
            }
            STATE(EscapeArg) {
                switch (ch) {
                    case L';': // End of arg
                        SWITCH_STATE(EscapeArgEnd);
                        break;
                    default:
                        buffer.sputc(ch);
                        break;
                }
                break;
            }
            STATE(EscapeArgEnd) {
                switch (escapeCode) {
                    case L'f': // Font
                        currentFontName = narrowToWideCharConverter.to_bytes(buffer.str());
                        break;
                    case L'c': // RGB(A) color
                    {
                        auto colorCode = narrowToWideCharConverter.to_bytes(buffer.str());
                        uint32_t temp;

                        try {
                            switch (colorCode.size()) {
                                case 3: // RGB -> RRGGBB
                                {
                                    temp = std::stoul(colorCode, 0, 16);
                                    uint32_t red = (temp & 0xF00) >> 8;
                                    uint32_t green = (temp & 0x0F0) >> 4;
                                    uint32_t blue = (temp & 0x00F) >> 0;

                                    currentColor = (
                                        red << 28 |
                                        red << 24 |
                                        green << 20 |
                                        green << 16 |
                                        blue << 12 |
                                        blue << 8 |
                                        0xFF
                                    );

                                    break;
                                }
                                case 6: // RRGGBB
                                    temp = std::stoul(colorCode, 0, 16);
                                    
                                    currentColor = temp << 8 | 0xFF;
                                    break;
                                case 8: // RRGGBBAA
                                    currentColor = std::stoul(colorCode, 0, 16);
                                    break;
                            }
                            break;
                        } catch (std::invalid_argument) {
                            // Ignore
                        }
                    }
                    default:
                        // Should never reach this
                        assert(false);
                        break;
                }
                buffer = std::wstringbuf();

                SWITCH_STATE_NO_CONSUME(Text);
                break;
            }
        }
    }
}

void Drawer::draw(const std::vector<Vertex> &vertices, const std::vector<GuiBufferInt> &indices, const Texture &texture) {
    // Create a new one every time the texture array switches to maintain vertex ordering
    Region *region;
    if (!currentRegion || currentRegion->textureArrayId != texture.arrayId) {
        region = &regions.emplace_back(Region{texture.arrayId});
        currentRegion = region;
    } else {
        region = currentRegion;
    }

    // Draw a rectangle
    GuiBufferInt startVertex = region->vertices.size();

    for (auto &vertex : vertices) {
        region->vertices.push_back(transformVertex(vertex));
    }

    for (auto &index : indices) {
        region->indices.push_back(index + startVertex);
    }
}

void Drawer::reset() {
    regions.clear();
    transformStack.clear();
    resetTransform();
}

void Drawer::translate(float x, float y) {
    // transform = glm::translate(transform, {x, y, 0});
    transform = glm::translate(transform, {x, y, 0});
}

void Drawer::rotate(float angle) {
    transform = glm::rotate(transform, angle, {0, 0, 1});
}

void Drawer::scale(float x, float y) {
    transform = glm::scale(transform, {x, y, 1});
}

void Drawer::scale(float all) {
    transform = glm::scale(transform, {all, all, 1});
}

void Drawer::resetTransform() {
    transform = glm::mat4{ 1.0f };
}

void Drawer::pushTransform(bool reset) {
    transformStack.push_back(transform);
    if (reset) {
        resetTransform();
    }
}

void Drawer::popTransform() {
    if (!transformStack.empty()) {
        transform = transformStack.back();
        transformStack.pop_back();
    }
}

Vertex Drawer::transformVertex(const Vertex &vertex) {
    auto posT = transform * glm::vec4(vertex.pos, 1);

    return {
        posT,
        vertex.normal,
        vertex.color,
        vertex.texCoord
    };
}

}