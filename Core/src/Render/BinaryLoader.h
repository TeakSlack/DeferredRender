#ifndef BINARY_LOADER_H
#define BINARY_LOADER_H

#include "Util/Log.h"
#include <vector>
#include <filesystem>
#include <fstream>

class BinaryLoader
{
public:
	static std::vector<uint8_t> LoadBinary(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			std::string fullPath = std::filesystem::absolute(path).string();
			LOG_ERROR_TO("shader", "Cannot open shader: {}", fullPath);
			return {};
		}
		auto size = static_cast<size_t>(file.tellg());
		std::vector<uint8_t> data(size);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(data.data()), size);
		return data;
	}
};

#endif // BINARY_LOADER_H