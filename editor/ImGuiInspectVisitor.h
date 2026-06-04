#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/Entity.h"
#include "ember/assets/AssetHandle.h"

#include <glm/glm.hpp>
#include <imgui.h>

#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

namespace ember {

// Reflection visitor that renders one ImGui widget per EMBER_FIELD. Mirrors the
// type coverage of the YAML visitors. `changed()` reports whether any field was
// edited this frame (so the panel can flag the scene dirty).
struct ImGuiInspectVisitor {
    bool m_changed = false;
    [[nodiscard]] bool changed() const { return m_changed; }
    void mark(bool c) { if (c) m_changed = true; }

    void operator()(const char* n, f32&  v) { mark(ImGui::DragFloat(n, &v, 0.05f)); }
    void operator()(const char* n, i32&  v) { mark(ImGui::DragInt(n, &v)); }
    void operator()(const char* n, u32&  v) { mark(ImGui::DragScalar(n, ImGuiDataType_U32, &v)); }
    void operator()(const char* n, u64&  v) { mark(ImGui::DragScalar(n, ImGuiDataType_U64, &v)); }
    void operator()(const char* n, bool& v) { mark(ImGui::Checkbox(n, &v)); }

    void operator()(const char* n, std::string& v) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "%s", v.c_str());
        if (ImGui::InputText(n, buf, sizeof(buf))) { v = buf; m_changed = true; }
    }

    void operator()(const char* n, glm::vec2& v) { mark(ImGui::DragFloat2(n, &v.x, 0.05f)); }
    void operator()(const char* n, glm::vec3& v) { mark(ImGui::DragFloat3(n, &v.x, 0.05f)); }
    void operator()(const char* n, glm::vec4& v) { mark(ImGui::DragFloat4(n, &v.x, 0.05f)); }

    void operator()(const char* n, Entity& v) {
        ImGui::Text("%s: Entity %llu", n,
                    static_cast<unsigned long long>(static_cast<std::underlying_type_t<Entity>>(v)));
    }
    void operator()(const char* n, std::vector<Entity>& v) {
        ImGui::Text("%s: [%zu entities]", n, v.size());
    }

    template<typename T>
    void operator()(const char* n, AssetHandle<T>& v) {
        ImGui::Text("%s: asset #%llu", n, static_cast<unsigned long long>(v.id));
    }

    template<typename E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
    void operator()(const char* n, E& v) {
        int iv = static_cast<int>(static_cast<std::underlying_type_t<E>>(v));
        if (ImGui::InputInt(n, &iv)) { v = static_cast<E>(iv); m_changed = true; }
    }
};

} // namespace ember
