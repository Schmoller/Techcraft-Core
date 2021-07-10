#include "tech-core/model.hpp"
#include "tech-core/mesh.hpp"
#include "tech-core/shapes/bounding_box.hpp"

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

    bool operator==(const tinyobj::index_t &rhs) const {
        return vertex_index == rhs.vertex_index &&
            texcoord_index == rhs.texcoord_index &&
            normal_index == rhs.normal_index;
    }
};

}

namespace std {
template<>
struct hash<Engine::exindex_t> {
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

    overallBounds = {};

    for (const auto &shape : shapes) {
        BoundingBox bounds {};
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
                vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2],
                };

                bounds.includeSelf(vertex.pos);

                uniqueVertices[exindex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
                indices.push_back(vertices.size() - 1);
            } else {
                indices.push_back(uniqueVertices[exindex]);
            }
        }

        subModels[shape.name] = {
            vertices,
            indices,
            bounds
        };

        overallBounds.includeSelf(bounds);
    }

    recomputeTangents();

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

void Model::getMeshData(
    const std::string &name, std::vector<Vertex> &outVertices, std::vector<uint32_t> &outIndices
) const {
    auto it = subModels.find(name);
    if (it == subModels.end()) {
        throw std::runtime_error("Unknown submodel");
    }

    auto &subModel = it->second;

    outVertices = subModel.vertices;
    outIndices = subModel.indices;
}

std::vector<std::string> Model::getSubModelNames() const {
    std::vector<std::string> names;
    names.reserve(subModels.size());

    for (auto &pair : subModels) {
        names.emplace_back(pair.first);
    }

    return names;
}

void Model::recomputeTangents() {
    for (auto &subModel : subModels) {
        recomputeTangents(subModel.second);
    }
}

void Model::recomputeTangents(Model::SubModel &subModel) {
    auto &vertices = subModel.vertices;
    auto &indices = subModel.indices;

    if (vertices.size() < 3 || indices.size() < 3) {
        return;
    }

    std::vector<glm::vec3> tangents(vertices.size(), { 0, 0, 0 });
    std::vector<uint32_t> tangentCounts(vertices.size(), 0);

    for (auto i = 0; i < indices.size(); i += 3) {
        Vertex &v1 = vertices[indices[i + 0]];
        Vertex &v2 = vertices[indices[i + 1]];
        Vertex &v3 = vertices[indices[i + 2]];

        glm::vec3 edge1, edge2;
        edge1 = v2.pos - v1.pos;
        edge2 = v3.pos - v1.pos;

        float s1 = v2.texCoord.x - v1.texCoord.x;
        float s2 = v3.texCoord.x - v1.texCoord.x;
        float t1 = v2.texCoord.y - v1.texCoord.y;
        float t2 = v3.texCoord.y - v1.texCoord.y;

        float r = 1.0f / (s1 * t2 - s2 * t1);

        glm::vec3 tangent {
            (t2 * edge1.x - t1 * edge2.x) * r,
            (t2 * edge1.y - t1 * edge2.y) * r,
            (t2 * edge1.z - t1 * edge2.z) * r
        };

        tangent = glm::normalize(tangent);

        // Add on the tangents
        tangents[indices[i + 0]] += tangent;
        tangents[indices[i + 1]] += tangent;
        tangents[indices[i + 2]] += tangent;

        // and update the counts
        tangentCounts[indices[i + 0]]++;
        tangentCounts[indices[i + 1]]++;
        tangentCounts[indices[i + 2]]++;
    }

    //now update the tangents
    for (auto i = 0; i < vertices.size(); ++i) {
        if (tangentCounts[i] == 0) {
            continue;
        }

        auto tangent = tangents[i] / static_cast<float>(tangentCounts[i]);

        // Gram-Schmidt orthogonalize
        tangent = (tangent - vertices[i].normal * glm::dot(vertices[i].normal, tangent));
        vertices[i].tangent = tangent;
    }
}

}