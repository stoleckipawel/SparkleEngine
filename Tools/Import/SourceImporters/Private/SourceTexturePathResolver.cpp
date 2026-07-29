#include "PCH.h"

#include "SourceTexturePathResolver.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/PathUtils.h"

#include <format>

std::filesystem::path SourceTexturePathResolver::ResolveExistingFile(
    const std::filesystem::path& sourceDirectory,
    std::string_view authoredPath)
{
	const std::optional<std::filesystem::path> resolvedPath =
	    Paths::ResolveRelativePath(sourceDirectory, std::filesystem::path(authoredPath));
	if (!resolvedPath)
	{
		throw Diagnostics::Error(std::format("Invalid texture path '{}'.", authoredPath));
	}

	std::error_code fileError;
	if (!std::filesystem::is_regular_file(*resolvedPath, fileError) || fileError)
	{
		throw Diagnostics::Error(std::format("Texture file does not exist: '{}'.", resolvedPath->string()));
	}

	const std::filesystem::path normalizedPath = Paths::Normalize(*resolvedPath);
	if (normalizedPath.empty())
	{
		throw Diagnostics::Error(std::format("Texture path cannot be normalized: '{}'.", resolvedPath->string()));
	}

	return normalizedPath;
}
