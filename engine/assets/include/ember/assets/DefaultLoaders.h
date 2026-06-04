#pragma once

namespace ember {

class AssetManager;

// Registers the built-in loaders (Texture2D, FontAsset, Shader) on the given
// manager. Call once at startup after creating the AssetManager.
void installDefaultLoaders(AssetManager& manager);

} // namespace ember
