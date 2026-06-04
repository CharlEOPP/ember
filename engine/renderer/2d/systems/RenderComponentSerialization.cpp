#include "ember/renderer/RenderComponents.h"
#include "ember/renderer/Components2D.h"
#include "ember/serialization/ComponentSerialization.h"

// This is the only renderer TU that pulls in yaml-cpp (via the serialization
// registration header). It bridges the renderer's reflected components to the
// scene serializer without the serialization module depending on the renderer.

namespace ember {

void registerRenderComponents() {
    registerComponentType<SpriteRenderer>("SpriteRenderer");
    registerComponentType<Camera2D>("Camera2D");
}

} // namespace ember
