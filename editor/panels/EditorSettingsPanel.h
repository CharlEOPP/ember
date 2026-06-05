#pragma once

namespace ember {

struct EditorSettings;

// Edit ▸ Preferences window. Returns true if any setting changed this frame
// (so the app can apply + persist). `open` is toggled by the window's close box.
class EditorSettingsPanel {
public:
    bool onImGuiRender(EditorSettings& settings, bool& open);
};

} // namespace ember
