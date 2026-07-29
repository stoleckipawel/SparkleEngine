#pragma once

#include <filesystem>

class ShaderRecookSignal final
{
  public:
	ShaderRecookSignal() = delete;

	static void Write(
	    const std::filesystem::path& registryReadPath,
	    const std::filesystem::path& publishedRegistryPath,
	    const std::filesystem::path& signalStoragePath);
};
