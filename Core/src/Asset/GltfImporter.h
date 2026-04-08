#ifndef GLTF_IMPORTER_H
#define GLTF_IMPORTER_H	

#include "Asset.h"
#include <vector>
#include <filesystem>

struct RawGltfPackage
{
	std::vector<MeshAsset> Meshes;
	std::vector<MaterialAsset> Materials;
	std::vector<TextureAsset> Textures;
};

class IGltfImporter
{
public:
	virtual ~IGltfImporter() = default;
	virtual RawGltfPackage Import(const std::filesystem::path& path) = 0;
};

class FastGltfImporter : public IGltfImporter
{
public:
	// TODO: import materials & textures
	RawGltfPackage Import(const std::filesystem::path& path) override;
};

#endif // GLTF_IMPORTER_H