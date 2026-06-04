#pragma once

namespace ember {

class AssetManager;

// Installs the global AssetHandle <-> virtual-path resolver used when (de)serializing
// component/script AssetHandle<T> fields, forwarding to `mgr` (pathOf / loadByType).
// Call once at startup; `mgr` must outlive any serialization that touches handles.
// Without this, handle fields serialize as "" and load back null (graceful).
void installAssetSerializationResolver(AssetManager& mgr);

} // namespace ember
