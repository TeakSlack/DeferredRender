#ifndef TEXTURE_IMPORTER_H
#define TEXTURE_IMPORTER_H

#include "AssetLoader.h"
#include "Asset.h"

class StbTextureImporter : public IAssetLoader<TextureAsset>
{
public:
    // Loads image from disk into TextureAsset::Data as RGBA8.
    // Sets Width, Height. Does NOT create GPU resources.
    std::unique_ptr<TextureAsset> Load(const std::filesystem::path& path) override;
};

#endif // TEXTURE_IMPORTER_H
