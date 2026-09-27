#include "PCH.h"

#include "Core/Public/Paths/ProductUserStatePaths.h"
#include "Core/Public/Paths/WorkspaceUserStatePaths.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "FileSystemDiscovery.h"
#include "UserStatePaths.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <Windows.h>
  #include <ShlObj.h>
#endif

namespace Filesystem::Private
{
	constexpr std::string_view FirstReleaseStateVersion = "v0.1";

	std::filesystem::path DiscoverPlatformLocalStateRoot()
	{
#if defined(_WIN32)
		PWSTR knownFolderPath = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &knownFolderPath)))
		{
			const std::filesystem::path result(knownFolderPath);
			CoTaskMemFree(knownFolderPath);
			return result;
		}
#else
		if (const char* stateHome = std::getenv("XDG_STATE_HOME"); stateHome != nullptr && *stateHome != '\0')
		{
			return std::filesystem::path(stateHome);
		}
		if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0')
		{
			return std::filesystem::path(home) / ".local" / "state";
		}
#endif
		std::error_code errorCode;
		const std::filesystem::path temporaryRoot = std::filesystem::temp_directory_path(errorCode);
		if (!errorCode)
		{
			return temporaryRoot;
		}
		errorCode.clear();
		return std::filesystem::current_path(errorCode);
	}

	std::filesystem::path ResolveSparkleUserStateRoot()
	{
		return DiscoverPlatformLocalStateRoot() / "SparkleEngine";
	}

	std::string MakeWorkspaceStateKey(const std::filesystem::path& workspaceRoot)
	{
		const std::filesystem::path normalizedRoot = Paths::Normalize(workspaceRoot);
		std::string pathKey = normalizedRoot.generic_string();
#if defined(_WIN32)
		for (char& character : pathKey)
		{
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
		}
#endif
		std::uint64_t hash = 14695981039346656037ull;
		for (const unsigned char character : pathKey)
		{
			hash ^= character;
			hash *= 1099511628211ull;
		}

		std::string workspaceName = Paths::MakeSafePathComponent(normalizedRoot.filename().string());
		if (workspaceName.empty())
		{
			workspaceName = "Workspace";
		}

		std::ostringstream result;
		result << workspaceName << '-' << std::hex << std::setfill('0') << std::setw(16) << hash;
		return result.str();
	}

	std::string ResolveProductName(const std::filesystem::path& projectRoot)
	{
		if (!projectRoot.empty())
		{
			return projectRoot.filename().string();
		}
		const std::string inferredName = InferProjectNameFromExecutableStem(GetExecutableStem());
		return inferredName.empty() ? "Sparkle" : inferredName;
	}

	ProductUserStatePaths BuildProductUserStatePaths(const std::filesystem::path& root)
	{
		const std::filesystem::path normalizedRoot = Paths::Normalize(root);
		return ProductUserStatePaths{
		    .Root = normalizedRoot,
		    .SettingsRoot = normalizedRoot / "Settings",
		    .LogsRoot = normalizedRoot / "Logs",
		    .CapturesRoot = normalizedRoot / "Captures",
		    .CrashesRoot = normalizedRoot / "Crashes",
		    .CacheRoot = normalizedRoot / "Cache"};
	}
}

namespace Filesystem
{
	WorkspaceUserStatePaths ResolveWorkspaceUserStatePaths(const std::filesystem::path& workspaceRoot)
	{
		const std::filesystem::path root = Private::ResolveSparkleUserStateRoot();
		const std::string workspaceKey = Private::MakeWorkspaceStateKey(workspaceRoot);
		return WorkspaceUserStatePaths{
		    .DevelopmentProductsRoot = root / "Development" / workspaceKey,
		    .LauncherRoot = root / "LauncherState" / workspaceKey};
	}

	ProductUserStatePaths ResolveDevelopmentProductUserStatePaths(
	    const std::filesystem::path& workspaceRoot,
	    std::string_view productName)
	{
		std::string safeProductName = Paths::MakeSafePathComponent(productName);
		if (safeProductName.empty())
		{
			safeProductName = "Sparkle";
		}
		const WorkspaceUserStatePaths workspaceState = ResolveWorkspaceUserStatePaths(workspaceRoot);
		return Private::BuildProductUserStatePaths(workspaceState.DevelopmentProductsRoot / safeProductName);
	}

	ProductUserStatePaths Private::ResolveCurrentProductUserStatePaths()
	{
		if (const auto packageRoot = Private::DiscoverPackageRoot())
		{
			const auto projectRoot = Private::DiscoverPackageProjectRoot(*packageRoot);
			return Private::BuildProductUserStatePaths(
			    Private::ResolveSparkleUserStateRoot()
			    / Private::ResolveProductName(projectRoot.value_or(std::filesystem::path{}))
			    / Private::FirstReleaseStateVersion);
		}

		const std::filesystem::path workspaceRoot = ResolveWorkspaceRootPath();
		const auto projectRoot = DiscoverProjectRoot();
		return ResolveDevelopmentProductUserStatePaths(
		    workspaceRoot,
		    Private::ResolveProductName(projectRoot.value_or(std::filesystem::path{})));
	}
}
