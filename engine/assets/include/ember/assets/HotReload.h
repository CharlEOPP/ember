#pragma once

namespace ember {

class AssetManager;
class FileWatcher;
class EventBus;

// Drain the watcher's changed paths, reload affected assets, and (if `bus` is
// non-null) post an AssetReloadedEvent for each reloaded file. Call once per
// frame on the main thread. No-op in Release where the watcher is inactive.
void pollAssetHotReload(AssetManager& manager, FileWatcher& watcher, EventBus* bus = nullptr);

} // namespace ember
