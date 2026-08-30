#pragma once

#include <filesystem>

class ShaderRecookSignal final
{
public:
	ShaderRecookSignal() = delete;

	static void Write(
	    const std::filesystem::path& mapReadPath,
	    const std::filesystem::path& publishedMapPath,
	    const std::filesystem::path& libraryReadPath,
	    const std::filesystem::path& publishedLibraryPath,
	    const std::filesystem::path& signalStoragePath);
};
