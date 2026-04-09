#ifndef GLTF_IMPORTER_H
#define GLTF_IMPORTER_H

#include "Asset.h"
#include <vector>
#include <filesystem>

// Intermediate material representation using image indices instead of asset handles.
// AssetSystem::LoadGltf() resolves indices to handles after registering textures.
struct RawMaterialData
{
	std::string Name;

	int AlbedoMapIndex         = -1;
	int NormalMapIndex         = -1;
	int MetallicRoughnessIndex = -1;
	int OcclusionMapIndex      = -1;
	int EmissiveMapIndex       = -1;

	Vector4 BaseColorFactor   { 1.0f };
	float   MetallicFactor     = 1.0f;
	float   RoughnessFactor    = 1.0f;
	Vector3 EmissiveFactor    { 0.0f };
	float   NormalScale        = 1.0f;
	float   OcclusionStrength  = 1.0f;

	MaterialAsset::AlphaMode Alpha = MaterialAsset::AlphaMode::Opaque;
	float AlphaCutoff = 0.5f;
	bool  DoubleSided = false;
};

struct RawGltfPackage
{
	std::vector<MeshAsset>       Meshes;
	std::vector<int>             MeshMaterialIndices; // parallel to Meshes; index into Materials, -1 if none
	std::vector<RawMaterialData> Materials;
	std::vector<TextureAsset>    Textures; // indexed by image index, parallel to glTF images[]
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
	RawGltfPackage Import(const std::filesystem::path& path) override;
};

#endif // GLTF_IMPORTER_H