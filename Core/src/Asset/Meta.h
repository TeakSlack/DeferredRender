#ifndef META_H
#define META_H

#include <unordered_map>
#include <filesystem>

#include "Asset/Asset.h"

struct MetaFile
{
	AssetID RootGuid;
	std::unordered_map<std::string, AssetID> SubAssets;

	bool HasSubAsset(const std::string& name) const
	{
		return SubAssets.find(name) != SubAssets.end();
	}

	AssetID GetOrCreate(const std::string& name)
	{
		auto it = SubAssets.find(name);
		if (it != SubAssets.end())
			return it->second;
		AssetID newId = AssetID::Generate();
		SubAssets[name] = newId;
		return newId;
	}

	static MetaFile LoadOrCreate(const std::filesystem::path& metaPath);
	void Save(const std::filesystem::path& metaPath) const;
};

#endif // META_H