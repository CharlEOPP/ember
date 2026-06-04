#include "ember/scripting/ScriptRegistry.h"

namespace ember {

ScriptRegistry& ScriptRegistry::instance() {
    static ScriptRegistry s_instance;
    return s_instance;
}

void ScriptRegistry::registerScript(ScriptTypeInfo info) {
    if (byName(info.name)) return;   // idempotent across TUs (REG-02)
    m_scripts.push_back(std::move(info));
}

void ScriptRegistry::setOrder(std::string_view name, i32 order) {
    for (auto& s : m_scripts)
        if (s.name == name) { s.order = order; return; }
}

const ScriptTypeInfo* ScriptRegistry::byName(std::string_view name) const {
    for (const auto& s : m_scripts)
        if (s.name == name) return &s;
    return nullptr;
}

void ScriptRegistry::clear() { m_scripts.clear(); }

} // namespace ember
