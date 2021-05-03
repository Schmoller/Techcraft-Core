#include "tech-core/model.hpp"
#include "tech-core/mesh.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <stdexcept>
#include <unordered_map>
#include <iostream>

namespace Engine {

// Hash extensions for index_t
struct exindex_t : tinyobj::index_t {
    exindex_t(const tinyobj::index_t &rhs) {
        vertex_index = rhs.vertex_index;
        texcoord_index = rhs.texcoord_index;
        normal_index = rhs.normal_index;
    }

    bool operator==(const tinyobj::index_t& rhs) const {
        return vertex_index == rhs.vertex_index &&
            texcoord_index == rhs.texcoord_index &&
            normal_index == rhs.normal_index;
    }
};

}

namespace std {
    template<> struct hash<Engine::exindex_t> {
        size_t operator()(Engine::exindex_t const &index) const {
            return ((hash<int>()(index.vertex_index) ^
                (hash<int>()(index.texcoord_index) << 1)) >> 1) ^
                (hash<int>()(index.normal_index) << 1);
        }
    };
}

namespace Engine {

bool loadModel(const std::string &path, StaticMeshBuilder<Vertex> &meshBuilder) {
    Model model;

    if (!model.load(path)) {
        return false;
    }

    model.applyCombined(meshBuilder);

    return true;
}

Model::Model() {}

Model::Model(const std::string &path) {
    load(path);
}

bool Model::load(const std::string &path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::cout << "Loading model " << path << std::endl;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        // TODO: Change away from using exceptions
        throw std::runtime_error(warn + err);
    }

    for (const auto &shape : shapes) {
        std::unordered_map<exindex_t, uint32_t> uniqueVertices = {};
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        for (const auto &index : shape.mesh.indices) {
            exindex_t exindex(index);

            if (uniqueVertices.count(exindex) == 0) {
                // Need to create the vertex
                Vertex vertex = {};
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                };
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
                vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2],
                };

                uniqueVertices[exindex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
                indices.push_back(vertices.size() - 1);
            } else {
                indices.push_back(uniqueVertices[exindex]);
            }
        }

        subModels[shape.name] = {
            vertices,
            indices
        };
    }

    return true;
}

void Model::applyCombined(StaticMeshBuilder<Vertex> &meshBuilder) const {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    size_t totalVertices = 0;
    size_t totalIndices = 0;

    // Compute memory needs
    for (auto &pair : subModels) {
        auto &subModel = pair.second;
        totalVertices += subModel.vertices.size();
        totalIndices += subModel.indices.size();
    }

    vertices.reserve(totalVertices);
    indices.reserve(totalIndices);


    for (auto &pair : subModels) {
        auto &subModel = pair.second;
        size_t startIndex = vertices.size();
        vertices.insert(vertices.end(), subModel.vertices.begin(), subModel.vertices.end());

        for (auto index : subModel.indices) {
            indices.push_back(index + startIndex);
        }
    }

    meshBuilder.withVertices(vertices);
    meshBuilder.withIndices(indices);
}

void Model::applySubModel(StaticMeshBuilder<Vertex> &meshBuilder, const std::string &name) const {
    auto it = subModels.find(name);
    if (it == subModels.end()) {
        throw std::runtime_error("Unknown submodel");
    }

    auto &subModel = it->second;

    meshBuilder.withVertices(subModel.vertices);
    meshBuilder.withIndices(subModel.indices);
}

}