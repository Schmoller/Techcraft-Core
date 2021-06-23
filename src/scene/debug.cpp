#include "tech-core/scene/debug.hpp"
#include "tech-core/scene/scene.hpp"
#include "tech-core/scene/entity.hpp"
#include "tech-core/debug.hpp"
#include <imgui.h>
#include <glm/glm.hpp>
#include <scene/components/planner_data.hpp>
#include <tech-core/scene/components/mesh_renderer.hpp>
#include <tech-core/mesh.hpp>

namespace Engine {

void showSceneTree(Scene &, std::weak_ptr<Entity> &selected);
void makeEntityTree(const std::shared_ptr<Entity> &, std::weak_ptr<Entity> &selected);
void showEntityInformation(Entity *);
void displayMatrix(const glm::mat4 &);

void showSceneDebugUI(Scene &scene) {
    static std::weak_ptr<Entity> selected;

    ImGui::Begin("Scene debug");

    ImGui::BeginChild("Entities", ImVec2(180, 0), true);
    showSceneTree(scene, selected);
    ImGui::EndChild();
    ImGui::SameLine();

    // Show info
    ImGui::BeginGroup();
    auto selectedEntity = selected.lock();
    if (selectedEntity) {
        showEntityInformation(selectedEntity.get());
    }
    ImGui::EndGroup();

    ImGui::End();
};

void showSceneTree(Scene &scene, std::weak_ptr<Entity> &selected) {
    for (auto &child : scene.getChildren()) {
        makeEntityTree(child, selected);
    }
}

void makeEntityTree(const std::shared_ptr<Entity> &entity, std::weak_ptr<Entity> &selected) {
    const char *extra = "";
    if (selected.lock() == entity) {
        extra = " [X]";
    }

    if (ImGui::TreeNode(reinterpret_cast<void *>(entity->getId()), "Entity %d%s", entity->getId(), extra)) {
        if (ImGui::IsItemClicked()) {
            selected = entity;
        }

        for (auto &child : entity->getChildren()) {
            makeEntityTree(child, selected);
        }
        ImGui::TreePop();
    } else {
        if (ImGui::IsItemClicked()) {
            selected = entity;
        }
    }
}

void showEntityInformation(Entity *entity) {
    auto pos = entity->getTransform().getPosition();
    auto rotation = entity->getTransform().getRotation();
    auto scale = entity->getTransform().getScale();

    drawAABB(pos - scale / 2.0f, pos + scale / 2.0f);

    if (ImGui::CollapsingHeader("Transform")) {
        if (ImGui::DragFloat3("Position", &pos.x)) {
            entity->getTransform().setPosition(pos);
        }
        if (ImGui::DragFloat4("Rotation", &rotation.x)) {
            entity->getTransform().setRotation(rotation);
        }
        if (ImGui::DragFloat3("Scale", &scale.x, 0.01)) {
            entity->getTransform().setScale(scale);
        }
    }

    if (entity->has<Internal::PlannerData>() && ImGui::CollapsingHeader("PlannerData")) {
        auto &data = entity->get<Internal::PlannerData>();

        ImGui::Text("Absolute transform:");
        displayMatrix(data.absoluteTransform);

        ImGui::Text("Renderable? %d", data.render.buffer != nullptr);

        ImGui::Text("Render Buffer: %d", data.render.buffer ? data.render.buffer->id : -1);
        ImGui::Text("Render Offset: %lu", data.render.buffer ? data.render.uniformOffset : 0);
    }

    if (entity->has<MeshRenderer>() && ImGui::CollapsingHeader("MeshRenderer")) {
        auto &data = entity->get<MeshRenderer>();
        ImGui::Text("Mesh: %ul", data.getMesh());
    }
}

void displayMatrix(const glm::mat4 &matrix) {
    if (ImGui::BeginTable("##Some Matrix", 4, ImGuiTableFlags_Borders)) {
        for (int row = 0; row < 4; ++row) {
            ImGui::TableNextRow();
            for (int col = 0; col < 4; ++col) {
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", matrix[row][col]);
            }
        }
        ImGui::EndTable();
    }
}
}