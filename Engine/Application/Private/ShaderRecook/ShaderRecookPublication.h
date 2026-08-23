#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

struct ShaderRecookPublication final
{
	std::uint64_t PublicationId = 0;
	std::uint64_t PublishedAtUnixMs = 0;
	std::uint64_t GlobalShaderMapHash = 0;
	std::uint64_t CookedShaderLibraryHash = 0;
	std::filesystem::path GlobalShaderMapPath;
	std::filesystem::path CookedShaderLibraryPath;
	std::string Status;
};

struct ShaderRecookPublicationReadResult final
{
	std::optional<ShaderRecookPublication> Publication;
	std::string Diagnostic;
	bool Missing = false;
};
