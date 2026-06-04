#include "EditorInspectors.h"
#include "InspectorRegistry.h"
#include "ImGuiInspectVisitor.h"

#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"
#include "ember/renderer/Components2D.h"
#include "ember/renderer/SpriteAnimation.h"
#include "ember/renderer/ParticleEmitter.h"
#include "ember/physics2d/RigidBody2D.h"
#include "ember/physics2d/Colliders2D.h"

#include <imgui.h>
#include <glm/glm.hpp>

namespace ember {

// Generic reflection-driven draw for any reflected component.
template<typename T>
static bool drawReflected(World& w, Entity e) {
    ImGuiInspectVisitor v;
    reflect(w.get<T>(e), v);
    return v.changed();
}

// ---- Hand-written overrides ----

static bool drawTransform(World& w, Entity e) {
    Transform& t = w.get<Transform>(e);
    bool changed = false;
    changed |= ImGui::DragFloat3("position", &t.position.x, 0.05f);
    ImGui::SameLine(); if (ImGui::SmallButton("R##pos")) { t.position = glm::vec3(0.0f); changed = true; }

    float deg = glm::degrees(t.rotation);
    if (ImGui::DragFloat("rotation", &deg, 0.5f)) { t.rotation = glm::radians(deg); changed = true; }
    ImGui::SameLine(); if (ImGui::SmallButton("R##rot")) { t.rotation = 0.0f; changed = true; }

    changed |= ImGui::DragFloat3("scale", &t.scale.x, 0.05f);
    ImGui::SameLine(); if (ImGui::SmallButton("R##scl")) { t.scale = glm::vec3(1.0f); changed = true; }
    return changed;
}

static bool drawSpriteRenderer(World& w, Entity e) {
    SpriteRenderer& s = w.get<SpriteRenderer>(e);
    bool changed = false;
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", s.texturePath.c_str());
    if (ImGui::InputText("texture", buf, sizeof(buf))) { s.texturePath = buf; changed = true; }
    // (Phase 8: an Asset Browser drag-drop target also assigns this.)
    changed |= ImGui::ColorEdit4("color", &s.color.x);
    changed |= ImGui::DragInt("layer", &s.layer);
    changed |= ImGui::Checkbox("flipX", &s.flipX); ImGui::SameLine();
    changed |= ImGui::Checkbox("flipY", &s.flipY);
    changed |= ImGui::DragFloat4("uvRect", &s.uvRect.x, 0.01f);
    return changed;
}

static bool drawCamera2D(World& w, Entity e) {
    Camera2D& c = w.get<Camera2D>(e);
    bool changed = false;
    changed |= ImGui::DragFloat("size", &c.size, 0.05f, 0.01f, 1000.0f);
    changed |= ImGui::DragFloat("nearClip", &c.nearClip, 0.05f);
    changed |= ImGui::DragFloat("farClip", &c.farClip, 0.05f);
    changed |= ImGui::Checkbox("isPrimary", &c.isPrimary);
    return changed;
}

void registerEditorInspectors() {
    // Overrides for the most-used components.
    registerInspector<Transform>("Transform", drawTransform);
    registerInspector<SpriteRenderer>("SpriteRenderer", drawSpriteRenderer);
    registerInspector<Camera2D>("Camera2D", drawCamera2D);

    // Reflection defaults for the rest.
    registerInspector<Tag>("Tag", drawReflected<Tag>);
    registerInspector<SpriteAnimator>("SpriteAnimator", drawReflected<SpriteAnimator>);
    registerInspector<ParticleEmitter>("ParticleEmitter", drawReflected<ParticleEmitter>);
    registerInspector<RigidBody2D>("RigidBody2D", drawReflected<RigidBody2D>);
    registerInspector<BoxCollider2D>("BoxCollider2D", drawReflected<BoxCollider2D>);
    registerInspector<CircleCollider2D>("CircleCollider2D", drawReflected<CircleCollider2D>);
}

} // namespace ember
