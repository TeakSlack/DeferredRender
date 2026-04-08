#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <memory>
#include <filesystem>

template<typename T>
class IAssetLoader
{
public:
	virtual ~IAssetLoader() = default;
	virtual std::unique_ptr<T> Load(const std::filesystem::path& path) = 0;
};

#endif // ASSET_LOADER_H