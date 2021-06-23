#pragma once

#include "../forward.hpp"
#include "../types.hpp"
#include "tech-core/utilities/badge.hpp"
#include <memory>
#include <vector>
#include <unordered_map>

namespace Engine {

class Scene {
public:
    void addChild(const std::shared_ptr<Entity> &entity);
    std::shared_ptr<Entity> getChildById(EntityId) const;
    std::shared_ptr<Entity> getChildByIndex(uint32_t) const;
    void removeChildById(EntityId);
    void removeChildByIndex(uint32_t);

    const std::vector<std::shared_ptr<Entity>> &getChildren() const { return children; }

    void onAdd(Badge<Entity>, const std::shared_ptr<Entity> &);
    void onRemove(Badge<Entity>, const std::shared_ptr<Entity> &);
    void onInvalidate(Badge<Entity>, Entity *, int type);

    void onSetActive(Badge<RenderEngine>, Internal::RenderPlanner *);
    void onSetInactive(Badge<RenderEngine>);

private:
    Internal::RenderPlanner *renderPlanner { nullptr };

    std::vector<std::shared_ptr<Entity>> children;
    std::unordered_map<EntityId, std::shared_ptr<Entity>> childrenById;
};

}



