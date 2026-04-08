#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "TextureImporter.h"
#include "Util/Log.h"

std::unique_ptr<TextureAsset> StbTextureImporter::Load(const std::filesystem::path& path)
{
    int width, height, channels;

    // Force RGBA8 — simplifies GPU upload (single format, no channel guessing)
    stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels)
    {
        CORE_WARN("[TextureImporter] Failed to load: {} — {}", path.string(), stbi_failure_reason());
        return nullptr;
    }

    auto asset      = std::make_unique<TextureAsset>();
    asset->Width    = static_cast<uint32_t>(width);
    asset->Height   = static_cast<uint32_t>(height);

    const size_t byteSize = static_cast<size_t>(width) * height * 4;
    asset->Data.assign(pixels, pixels + byteSize);

    stbi_image_free(pixels);

    CORE_INFO("[TextureImporter] Loaded {}x{} ({} ch → RGBA8): {}",
        width, height, channels, path.filename().string());

    return asset;
}
