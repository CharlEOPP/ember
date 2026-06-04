#include "InspectorRegistry.h"

namespace ember {

InspectorRegistry& InspectorRegistry::instance() {
    static InspectorRegistry reg;
    return reg;
}

void InspectorRegistry::add(InspectorEntry e) {
    for (const InspectorEntry& existing : m_entries)
        if (existing.type == e.type) return;   // idempotent
    m_entries.push_back(std::move(e));
}

const InspectorEntry* InspectorRegistry::byType(std::type_index t) const {
    for (const InspectorEntry& e : m_entries)
        if (e.type == t) return &e;
    return nullptr;
}

void InspectorRegistry::clear() {
    m_entries.clear();
}

} // namespace ember
