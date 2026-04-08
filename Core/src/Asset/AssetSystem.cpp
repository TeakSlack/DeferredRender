#include "AssetSystem.h"
#include "Meta.h"
#include "GltfImporter.h"
#include "Util/Log.h"
#include <thread>

AssetSystem& AssetSystem::Get()
{
    static AssetSystem s_Instance;
	return s_Instance;
}

void AssetSystem::Init()
{
	m_AssetRoot = std::filesystem::current_path() / "Assets";

	const size_t workerCount = std::max(1u, std::thread::hardware_concurrency() - 1);
	m_ThreadPool = std::make_unique<ThreadPool>(workerCount);

	// Register pre-existing meta files
    ScanMetaFiles();

	LOG_INFO_TO("asset", "initialized. Asset root {} ({} workers)", m_AssetRoot.string(), workerCount);
}

void AssetSystem::Shutdown()
{
	// Destroy the pool first — joins all workers so no CompletedJob writes can
	// race with the m_Assets destructor below.
	m_ThreadPool.reset();
}

void AssetSystem::Tick(float)
{
	std::lock_guard lock(m_CompletedMutex);
	while (!m_CompletedJobs.empty())
	{
		auto& job = m_CompletedJobs.front();
		auto it   = m_Assets.find(job.id);
		if (it != m_Assets.end())
		{
			it->second.data  = std::move(job.data);
			it->second.state = job.state;
		}
		m_CompletedJobs.pop();
	}
}

GltfObject AssetSystem::LoadGltf(const std::filesystem::path& path)
{
    std::filesystem::path fullPath = std::filesystem::weakly_canonical(m_AssetRoot / path);

    // --- Deduplication ---
    auto it = m_PathToID.find(fullPath);
    if (it != m_PathToID.end())
    {
        LOG_INFO_TO("asset", "GLTF already loaded : {}", fullPath.string());
        
        // Check is assets are already in memory
        bool loaded = false;
        for (auto& [id, record] : m_Assets)
        {
            if (record.sourcePath == fullPath)
            {
                loaded = true;
                break;
            }
        }

        if (loaded)
        {
            GltfObject result;
            for (auto& [id, record] : m_Assets)
            {
				if (record.sourcePath == fullPath && record.type == typeid(MeshAsset))
				{
					result.Meshes.push_back(AssetHandle<MeshAsset>{ id });
				}
            }
            return result;
        }
    }

    // --- Fresh import ---
    if (!std::filesystem::exists(fullPath))
    {
        LOG_INFO_TO("asset", "File not found: {}", fullPath.string());
        return {};
    }

    MetaFile meta = MetaFile::LoadOrCreate(fullPath);

    FastGltfImporter importer;
    RawGltfPackage rawPackage = importer.Import(fullPath);

    GltfObject result;
    for (auto& mesh : rawPackage.Meshes)
    {
        AssetID meshId = meta.GetOrCreate("Mesh:" + mesh.Name);
        result.Meshes.push_back(AddAssetWithID<MeshAsset>(meshId, std::move(mesh), fullPath));
    }

    meta.Save(fullPath);
    return result;
}

void AssetSystem::ScanMetaFiles()
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

		MetaFile meta = MetaFile::LoadOrCreate(entry.path());
		if (!meta.RootGuid.IsValid())
		{
			LOG_WARN_TO("asset", "Failed to load meta file: {}", entry.path().string());
			continue;
		}

		m_PathToID[sourcePath] = meta.RootGuid;
	}
}
