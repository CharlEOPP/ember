#include "ember/assets/HotReload.h"
#include "ember/assets/AssetManager.h"
#include "ember/assets/FileWatcher.h"
#include "ember/events/EventBus.h"
#include "ember/events/Events.h"

namespace ember {

void pollAssetHotReload(AssetManager& manager, FileWatcher& watcher, EventBus* bus) {
    for (const auto& path : watcher.poll()) {
        const u32 n = manager.reloadByRealPath(path);
        if (n > 0 && bus) bus->post<AssetReloadedEvent>(path.string());
    }
}

} // namespace ember
