#include "ember/renderer/RenderComponents.h"
#include "ember/renderer/Components2D.h"
#include "ember/renderer/SpriteAnimation.h"
#include "ember/renderer/ParticleEmitter.h"
#include "ember/renderer/UIComponents.h"
#include "ember/serialization/ComponentSerialization.h"

// This is the only renderer TU that pulls in yaml-cpp (via the serialization
// registration header). It bridges the renderer's reflected components to the
// scene serializer without the serialization module depending on the renderer.

namespace ember {

void registerRenderComponents() {
    registerComponentType<SpriteRenderer>("SpriteRenderer");
    registerComponentType<Camera2D>("Camera2D");
    registerComponentType<SpriteAnimator>("SpriteAnimator");
    registerComponentType<ParticleEmitter>("ParticleEmitter");
    registerComponentType<UIImage>("UIImage");
    registerComponentType<UIText>("UIText");
    registerComponentType<UIButton>("UIButton");
}

} // namespace ember
