#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

struct ShaderRecookSignalResult final
{
	std::filesystem::path signalPath;
	std::uint64_t registryHash = 0;
};

class ShaderRecookSignal final
{
  public:
	ShaderRecookSignal() = delete;

	static bool Write(
	    const std::filesystem::path& registryReadPath,
	    const std::filesystem::path& publishedRegistryPath,
	    const std::filesystem::path& signalStoragePath,
	    const std::filesystem::path& publishedSignalPath,
	    ShaderRecookSignalResult& outResult,
	    std::string& outErrorMessage);
};
