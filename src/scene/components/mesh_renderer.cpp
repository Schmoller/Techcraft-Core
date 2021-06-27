#include "tech-core/scene/components/mesh_renderer.hpp"
#include "tech-core/scene/entity.hpp"

namespace Engine {

MeshRenderer::MeshRenderer(Entity &owner)
    : Component(owner) {

}

void MeshRenderer::setMesh(const Mesh *newMesh) {
    mesh = newMesh;
    owner.invalidate(EntityInvalidateType::Render);
}

void MeshRenderer::setMaterial(const Material *newMaterial) {
    material = newMaterial;
    owner.invalidate(EntityInvalidateType::Render);
}

}
