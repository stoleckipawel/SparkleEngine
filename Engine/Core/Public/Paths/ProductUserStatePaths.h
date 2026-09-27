#pragma once

#include "Core/Public/CoreAPI.h"

#include <filesystem>
#include <string_view>

namespace Filesystem
{
	struct ProductUserStatePaths final
	{
		std::filesystem::path Root;
		std::filesystem::path SettingsRoot;
		std::filesystem::path LogsRoot;
		std::filesystem::path CapturesRoot;
		std::filesystem::path CrashesRoot;
		std::filesystem::path CacheRoot;
	};

	// Returns process-lifetime paths for the current product.
	SPARKLE_CORE_API const ProductUserStatePaths& GetProductUserStatePaths();

	// Resolves another development product's state for Launcher inspection/cleanup.
	SPARKLE_CORE_API ProductUserStatePaths ResolveDevelopmentProductUserStatePaths(
	    const std::filesystem::path& workspaceRoot,
	    std::string_view productName);
}
