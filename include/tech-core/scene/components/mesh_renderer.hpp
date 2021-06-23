#pragma once

#include "base.hpp"
#include "tech-core/forward.hpp"

namespace Engine {

class MeshRenderer final : public Component {
public:
    explicit MeshRenderer(Entity &);

    void setMesh(const Mesh *mesh);

    const Mesh *getMesh() const { return mesh; };

    // TODO: Materials
private:
    const Mesh *mesh {};
};

}
