#ifndef __MODEL_HPP
#define __MODEL_HPP

#include "forward.hpp"
#include "vertex.hpp"
#include "shapes/bounding_box.hpp"

#include <vk_mem_alloc.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace Engine {

bool loadModel(const std::string &path, StaticMeshBuilder<Vertex> &meshBuilder);

class Model {
public:
    explicit Model(const std::string &path);
    Model();

    bool load(const std::string &path);

    void applyCombined(StaticMeshBuilder<Vertex> &meshBuilder) const;
    void applySubModel(StaticMeshBuilder<Vertex> &meshBuilder, const std::string &name) const;

    void getMeshData(
        const std::string &subModel, std::vector<Vertex> &outVertices, std::vector<uint32_t> &outIndices
    ) const;
    std::vector<std::string> getSubModelNames() const;

    const BoundingBox &getBounds() const { return overallBounds; }

private:
    struct SubModel {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        BoundingBox bounds;
    };

    std::unordered_map<std::string, SubModel> subModels;

    BoundingBox overallBounds;

    void recomputeTangents();
    void recomputeTangents(SubModel &);
};

}

#endif 