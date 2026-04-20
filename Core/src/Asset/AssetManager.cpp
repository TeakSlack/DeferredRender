#include "AssetManager.h"
#include "Meta.h"
#include "GltfImporter.h"
#include "Util/Log.h"
#include <thread>

void AssetManager::Init()
{
	m_AssetRoot = std::filesystem::current_path() / "Assets";

	const size_t workerCount = std::max(1u, std::thread::hardware_concurrency() - 1);
	m_ThreadPool = std::make_unique<ThreadPool>(workerCount);

	// Register pre-existing meta files
    ScanMetaFiles();

	LOG_INFO_TO("asset", "initialized. Asset root {} ({} workers)", m_AssetRoot.string(), workerCount);
}

void AssetManager::Shutdown()
{
	// Destroy the pool first — joins all workers so no CompletedJob writes can
	// race with the m_Assets destructor below.
	m_ThreadPool.reset();
}

void AssetManager::Tick(float)
{
	std::queue<CompletedJob> local;
	{
		std::lock_guard lock(m_CompletedMutex);
		std::swap(local, m_CompletedJobs);
	}

	while (!local.empty())
	{
		auto& job = local.front();
		auto it   = m_Assets.find(job.id);
		if (it != m_Assets.end())
		{
			it->second.data  = std::move(job.data);
			it->second.state = job.state;
		}
		local.pop();
	}
}

GltfObject AssetManager::LoadGltf(const std::filesystem::path& path)
{
    std::filesystem::path fullPath = std::filesystem::weakly_canonical(m_AssetRoot / path);

    // --- Deduplication via cached GltfObject ---
    auto cachedIt = m_GltfObjects.find(fullPath);
    if (cachedIt != m_GltfObjects.end())
    {
        LOG_INFO_TO("asset", "GLTF already loaded: {}", fullPath.string());
        return cachedIt->second;
    }

    if (!std::filesystem::exists(fullPath))
    {
        LOG_WARN_TO("asset", "File not found: {}", fullPath.string());
        return {};
    }

    MetaFile meta = MetaFile::LoadOrCreate(fullPath);

    FastGltfImporter importer;
    RawGltfPackage rawPackage = importer.Import(fullPath);

    GltfObject result;

    // 1. Register textures — build an index-to-handle table for material binding.
    //    Textures are not assigned a sourcePath since they have no independent file
    //    identity within this load path; the GltfObject cache handles deduplication.
    std::vector<AssetHandle<TextureAsset>> textureHandles;
    textureHandles.reserve(rawPackage.Textures.size());
    for (size_t i = 0; i < rawPackage.Textures.size(); ++i)
    {
        AssetID id = meta.GetOrCreate("Texture:" + std::to_string(i));
        auto handle = AddAssetWithID<TextureAsset>(id, std::move(rawPackage.Textures[i]), {});
        textureHandles.push_back(handle);
        result.Textures.push_back(handle);
    }

    // 2. Register materials, resolving image indices to texture handles.
    auto bindTex = [&](int idx) -> AssetHandle<TextureAsset> {
        if (idx >= 0 && idx < static_cast<int>(textureHandles.size()))
            return textureHandles[idx];
        return {};
    };

    for (auto& rawMat : rawPackage.Materials)
    {
        MaterialAsset mat;
        mat.BaseColorFactor     = rawMat.BaseColorFactor;
        mat.MetallicFactor      = rawMat.MetallicFactor;
        mat.RoughnessFactor     = rawMat.RoughnessFactor;
        mat.EmissiveFactor      = rawMat.EmissiveFactor;
        mat.NormalScale         = rawMat.NormalScale;
        mat.OcclusionStrength   = rawMat.OcclusionStrength;
        mat.Alpha               = rawMat.Alpha;
        mat.AlphaCutoff         = rawMat.AlphaCutoff;
        mat.DoubleSided         = rawMat.DoubleSided;

        mat.AlbedoMap            = bindTex(rawMat.AlbedoMapIndex);
        mat.NormalMap            = bindTex(rawMat.NormalMapIndex);
        mat.MetallicRoughnessMap = bindTex(rawMat.MetallicRoughnessIndex);
        mat.OcclusionMap         = bindTex(rawMat.OcclusionMapIndex);
        mat.EmissiveMap          = bindTex(rawMat.EmissiveMapIndex);

        AssetID id = meta.GetOrCreate("Material:" + rawMat.Name);
        result.Materials.push_back(AddAssetWithID<MaterialAsset>(id, std::move(mat), {}));
    }

    // 3. Register meshes and build the parallel MeshMaterials array.
    result.MeshMaterials.resize(rawPackage.Meshes.size());
    for (size_t i = 0; i < rawPackage.Meshes.size(); ++i)
    {
        AssetID id = meta.GetOrCreate("Mesh:" + rawPackage.Meshes[i].Name);
        result.Meshes.push_back(AddAssetWithID<MeshAsset>(id, std::move(rawPackage.Meshes[i]), fullPath));

        int matIdx = (i < rawPackage.MeshMaterialIndices.size())
                   ? rawPackage.MeshMaterialIndices[i] : -1;
        if (matIdx >= 0 && static_cast<size_t>(matIdx) < result.Materials.size())
            result.MeshMaterials[i] = result.Materials[matIdx];
    }

    meta.SourceHash = MetaFile::ComputeHash(fullPath);
    meta.Save(fullPath);
    m_GltfObjects[fullPath] = result;
    return result;
}

void AssetManager::ScanMetaFiles()
{
	auto cacheRoot = m_AssetRoot / ".cache";
	if (!std::filesystem::exists(cacheRoot))
		return;

	for (auto& entry : std::filesystem::recursive_directory_iterator(cacheRoot))
	{
        if (entry.path().extension() != ".meta")
            continue;

		// Reconstruct source path from meta path: .cache/{relative_path}.meta -> {relative_path}
		std::filesystem::path relativePath = std::filesystem::relative(entry.path(), cacheRoot);
		std::filesystem::path sourcePath = m_AssetRoot / relativePath.parent_path() / relativePath.stem();

		MetaFile meta = MetaFile::LoadOrCreate(sourcePath);
		if (!meta.RootGuid.IsValid())
		{
			LOG_WARN_TO("asset", "Failed to load meta file: {}", entry.path().string());
			continue;
		}

		m_PathToID[sourcePath] = meta.RootGuid;
	}
}
