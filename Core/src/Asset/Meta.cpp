#include <fstream>
#include "Meta.h"
#include "Util/Log.h"
#include "AssetManager.h"

static std::filesystem::path MetaPath(const std::filesystem::path& sourcePath)
{
	AssetManager* assetManager = Engine::Get().GetSubmodule<AssetManager>();
    // e.g. assets/.cache/models/sword.gltf.meta
    auto relative = std::filesystem::relative(sourcePath, assetManager->GetAssetRoot());
    return assetManager->GetAssetRoot() / ".cache" / (relative.string() + ".meta");
}

MetaFile MetaFile::LoadOrCreate(const std::filesystem::path& sourcePath)
{
    MetaFile meta;
    auto metaPath = MetaPath(sourcePath);

    if (!std::filesystem::exists(metaPath))
    {
        meta.RootGuid = CoreUUID::Generate();
        meta.Save(sourcePath);
        return meta;
    }

    std::ifstream f(metaPath);
    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        AssetID     value = CoreUUID(std::stoull(line.substr(eq + 1)));
        if (key == "root")
            meta.RootGuid = value;
        else
            meta.SubAssets[key] = value;
    }
    return meta;
}

void MetaFile::Save(const std::filesystem::path& sourcePath) const
{
    auto metaPath = MetaPath(sourcePath);
    std::filesystem::create_directories(metaPath.parent_path());

    std::ofstream f(metaPath);
    if (!f)
    {
        CORE_WARN("[Meta] Failed to write meta file: {}", metaPath.string());
        return;
    }

    f << "root=" << RootGuid << '\n';
    for (auto& [name, id] : SubAssets)
        f << name << '=' << id << '\n';
}
