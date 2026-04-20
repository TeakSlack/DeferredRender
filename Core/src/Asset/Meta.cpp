#include <fstream>
#include <nlohmann/json.hpp>
#include "Meta.h"
#include "Util/Log.h"
#include "Util/Assert.h"
#include "AssetManager.h"

using namespace nlohmann;

static std::filesystem::path MetaPath(const std::filesystem::path& sourcePath)
{
	AssetManager* assetManager = Engine::Get().GetSubmodule<AssetManager>();
    // e.g. assets/.cache/models/sword.gltf.meta
    auto relative = std::filesystem::relative(sourcePath, assetManager->GetAssetRoot());
    return assetManager->GetAssetRoot() / ".cache" / (relative.string() + ".meta");
}

static void to_json(json& j, const GltfImportSettings& s)
{
	j = {
		{"Scale", s.Scale},
		{"ImportMeshes", s.ImportMeshes},
		{"ImportMaterials", s.ImportMaterials},
		{"ImportTextures", s.ImportTextures},
		{"ImportAnimations", s.ImportAnimations},
		{"GenerateLODs", s.GenerateLODs},
		{"LODCount", s.LODCount}
	};
}

static void to_json(json& j, const TextureImportSettings& s)
{
	j = {
		{"sRGB", s.sRGB},
		{"GenerateMipmaps", s.GenerateMipmaps},
		{"Compression", s.Compression},
		{"MaxSize", s.MaxSize},
		{"AddressU", s.AddressU},
		{"AddressV", s.AddressV}
	};
}

static void to_json(json& j, const MetaFile& m)
{
    j["Version"] = 1;
    j["Type"] = m.Type;
	j["RootGuid"] = m.RootGuid.Value();
	j["SourceHash"] = m.SourceHash;

	j["SubAssets"] = json::object();
	for (auto& [name, id] : m.SubAssets)
		j["SubAssets"][name] = id.Value();

	j["Dependencies"] = json::object();
	for (auto& dep : m.Dependencies)
		j["Dependencies"].push_back(dep.Value());

	if (m.Type == "Gltf")
		j["GltfSettings"] = m.GltfSettings;
	else if (m.Type == "Texture")
		j["TextureSettings"] = m.TextureSettings;
}

static void from_json(const json& j, GltfImportSettings& s)
{
	s.Scale = j.value("Scale", 1.0f);
	s.ImportMeshes = j.value("ImportMeshes", true);
	s.ImportMaterials = j.value("ImportMaterials", true);
	s.ImportTextures = j.value("ImportTextures", true);
	s.ImportAnimations = j.value("ImportAnimations", false);
	s.GenerateLODs = j.value("GenerateLODs", false);
	s.LODCount = j.value("LODCount", 0);
}

static void from_json(const json& j, TextureImportSettings& s)
{
	s.sRGB = j.value("sRGB", true);
	s.GenerateMipmaps = j.value("GenerateMipmaps", true);
	s.Compression = j.value("Compression", "BC7");
	s.MaxSize = j.value("MaxSize", 4096);
	s.AddressU = j.value("AddressU", "Repeat");
	s.AddressV = j.value("AddressV", "Repeat");
}

static void from_json(const json& j, MetaFile& m)
{
	int version = j.value("Version", 1);
	if (version != 1)
	{
		CORE_WARN("[Meta] Unsupported meta version: {}", version);
		return;
	}
	m.Type = j.value("Type", "");
	m.RootGuid = CoreUUID(j.value("RootGuid", 0ULL));
	CORE_ASSERT(m.RootGuid.IsValid(), "MetaFile has null RootGuid");
	m.SourceHash = j.value("SourceHash", 0ULL);
	if (j.contains("SubAssets"))
		for (auto& [name, id] : j["SubAssets"].items())
			m.SubAssets[name] = CoreUUID(id.get<uint64_t>());
	if (j.contains("Dependencies"))
		for (auto& dep : j["Dependencies"])
			m.Dependencies.push_back(CoreUUID(dep.get<uint64_t>()));
	if (m.Type == "Gltf" && j.contains("GltfSettings"))
		m.GltfSettings = j["GltfSettings"].get<GltfImportSettings>();
	else if (m.Type == "Texture" && j.contains("TextureSettings"))
		m.TextureSettings = j["TextureSettings"].get<TextureImportSettings>();
}

uint64_t MetaFile::ComputeHash(const std::filesystem::path& sourcePath)
{
    std::error_code ec;
    auto mtime = std::filesystem::last_write_time(sourcePath, ec);
    if (ec) return 0;
    auto size = std::filesystem::file_size(sourcePath, ec);
    if (ec) return 0;

    // Pack mtime as seconds since its epoch XOR'd with file size.
    // Collisions are astronomically unlikely for a reimport-detection use case.
    auto mtimeSec = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            mtime.time_since_epoch()).count());
    return mtimeSec ^ (size << 32) ^ (size >> 32);
}

bool MetaFile::IsDirty(const std::filesystem::path& sourcePath) const
{
    return SourceHash == 0 || SourceHash != ComputeHash(sourcePath);
}

AssetID MetaFile::GetOrCreate(const std::string& name)
{
    auto it = SubAssets.find(name);
    if (it != SubAssets.end())
        return it->second;
    AssetID newId = AssetID::Generate();
    SubAssets[name] = newId;
    return newId;
}

MetaFile MetaFile::LoadOrCreate(const std::filesystem::path& sourcePath)
{
    MetaFile meta;
    auto metaPath = MetaPath(sourcePath);

    if (!std::filesystem::exists(metaPath))
    {
        meta.RootGuid   = CoreUUID::Generate();
        meta.SourceHash = MetaFile::ComputeHash(sourcePath);
        meta.Save(sourcePath);
        return meta;
    }

    std::ifstream f(metaPath);
	json j = json::parse(f, nullptr, false);
	meta = j.get<MetaFile>();
    return meta;
}

void MetaFile::Save(const std::filesystem::path& sourcePath) const
{
	auto metaPath = MetaPath(sourcePath);
	std::filesystem::create_directories(metaPath.parent_path());

	std::ofstream f(metaPath);
	if (!f) { CORE_WARN("[Meta] Failed to write: {}", metaPath.string()); return; }

	nlohmann::json j = *this;
	f << j.dump(4) << '\n';
}
