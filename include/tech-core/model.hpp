#ifndef __MODEL_HPP
#define __MODEL_HPP

#include "forward.hpp"
#include "vertex.hpp"

#include <vk_mem_alloc.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace Engine {

bool loadModel(const std::string &path, StaticMeshBuilder<Vertex> &meshBuilder);

class Model {
public:
    Model(const std::string &path);
    Model();

    bool load(const std::string &path);

    void applyCombined(StaticMeshBuilder<Vertex> &meshBuilder) const;
    void applySubModel(StaticMeshBuilder<Vertex> &meshBuilder, const std::string &name) const;

private:
    struct SubModel {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    std::unordered_map<std::string, SubModel> subModels;
};

}

#endif 