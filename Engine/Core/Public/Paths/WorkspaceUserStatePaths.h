#pragma once

#include "Core/Public/CoreAPI.h"

#include <filesystem>

namespace Filesystem
{
	struct WorkspaceUserStatePaths final
	{
		std::filesystem::path DevelopmentProductsRoot;
		std::filesystem::path LauncherRoot;
	};

	SPARKLE_CORE_API WorkspaceUserStatePaths ResolveWorkspaceUserStatePaths(const std::filesystem::path& workspaceRoot);
}
