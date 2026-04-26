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
	    const std::filesystem::path& cacheDirectory,
	    const std::filesystem::path& registryPath,
	    ShaderRecookSignalResult& outResult,
	    std::string& outErrorMessage);
};