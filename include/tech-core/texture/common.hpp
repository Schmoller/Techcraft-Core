#pragma once

#include <string>
#include <exception>

namespace Engine {

enum MipType {
    NoMipmap = 0,
    StoredStandard,
    Generate
};

class TextureLoadError : public std::exception {
public:
    explicit TextureLoadError(std::string filename)
        : filename(std::move(filename)) {}

    virtual const char *what() const noexcept override {
        return filename.c_str();
    }

private:
    std::string filename;
};

}