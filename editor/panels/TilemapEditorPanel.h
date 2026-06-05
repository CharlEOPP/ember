#pragma once
#include "../EditorContext.h"

namespace ember {

// Shown when the selected entity has a Tilemap. Hosts the tile palette + paint
// tools; the actual painting happens in the Viewport (which reads ctx.tilemap).
class TilemapEditorPanel {
public:
    void onImGuiRender(EditorContext& ctx);
};

} // namespace ember
