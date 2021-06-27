#pragma once

#include "base.hpp"
#include "tech-core/forward.hpp"

namespace Engine {

class MeshRenderer final : public Component {
public:
    explicit MeshRenderer(Entity &);

    void setMesh(const Mesh *);
    void setMaterial(const Material *);

    const Mesh *getMesh() const { return mesh; }

    const Material *getMaterial() const { return material; }

private:
    const Mesh *mesh {};
    const Material *material {};
};

}
