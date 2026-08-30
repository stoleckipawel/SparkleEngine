#pragma once

#include <filesystem>
#include <string_view>

class SourceTexturePathResolver final
{
public:
	static std::filesystem::path ResolveExistingFile(const std::filesystem::path& sourceDirectory, std::string_view authoredPath);
};
