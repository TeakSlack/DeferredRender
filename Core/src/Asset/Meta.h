#ifndef META_H
#define META_H

#include <unordered_map>
#include <filesystem>

#include "Asset/Asset.h"

struct GltfImportSettings
{
	float Scale = 1.0f;
	bool ImportMeshes = true;
	bool ImportMaterials = true;
	bool ImportTextures = true;
	bool ImportAnimations = false;
	bool GenerateLODs = false;
	int LODCount = 0;
};

struct TextureImportSettings
{
	bool sRGB = true;
	bool GenerateMipmaps = true;
	std::string Compression = "BC7"; // "None", "BC1", "BC3", "BC7"
	uint32_t MaxSize = 4096;
	std::string AddressU = "Repeat"; // "Repeat", "Clamp", "Mirror"
	std::string AddressV = "Repeat";
};

class MetaFile
{
public:
	AssetID RootGuid;
	std::string Type; // "Gltf", "Texture", etc.
	std::unordered_map<std::string, AssetID> SubAssets;
	std::vector<AssetID> Dependencies;

	GltfImportSettings GltfSettings;
	TextureImportSettings TextureSettings;

	uint64_t SourceHash = 0; // mtime (seconds) ^ file_size — 0 means unknown

	// Returns true when the source file on disk no longer matches the stored hash.
	bool IsDirty(const std::filesystem::path& sourcePath) const;
	static uint64_t ComputeHash(const std::filesystem::path& sourcePath);

	inline bool HasSubAsset(const std::string& name) const
	{
		return SubAssets.find(name) != SubAssets.end();
	}

	AssetID GetOrCreate(const std::string& name);
	static MetaFile LoadOrCreate(const std::filesystem::path& metaPath);
	void Save(const std::filesystem::path& metaPath) const;
};

#endif // META_H