#include "ember/ecs/Components.h"

// Built-in component field reflection (EMBER_REFLECT) is defined in Components.h so
// it is header-visible and instantiable by the serializer. Registration into the
// ComponentRegistry (with YAML serialize/deserialize callbacks) is performed by the
// serialization module, which is the only layer that depends on yaml-cpp.
//
// This translation unit exists so ember_ecs has a compiled object for the component
// definitions and as a home for any future non-template component helpers.

namespace ember {
} // namespace ember
