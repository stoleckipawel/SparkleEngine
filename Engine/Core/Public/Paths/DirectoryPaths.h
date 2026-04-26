#pragma once

#include "Core/Public/FileSystemUtils.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

namespace Paths
{
	// Process directories
	inline const std::filesystem::path& WorkingDirectory()
	{
		return Filesystem::GetWorkingDirectory();
	}

	inline std::filesystem::path ExecutableDirectory()
	{
		return Filesystem::GetExecutableDirectory();
	}

	// Stateless root discovery. Use these from early bootstrap systems that must not initialize the cached path state.
	inline std::filesystem::path ResolveWorkspaceRoot()
	{
		return Filesystem::ResolveWorkspaceRootPath();
	}

	inline std::filesystem::path ResolveBuildOutputRoot()
	{
		return Filesystem::ResolveBuildOutputRootPath();
	}

	inline std::filesystem::path ResolveLogsRoot(bool ensureExists = false)
	{
		std::filesystem::path path = Filesystem::ResolveLogsRootPath();
		if (ensureExists)
		{
			std::error_code errorCode;
			std::filesystem::create_directories(path, errorCode);
		}
		return path;
	}

	// Cached repository roots
	inline const std::filesystem::path& WorkspaceRoot()
	{
		return Filesystem::GetWorkspaceRootPath();
	}

	inline const std::filesystem::path& BuildOutputRoot()
	{
		return Filesystem::GetBuildOutputRootPath();
	}

	inline const std::filesystem::path& LogsRoot()
	{
		return Filesystem::GetLogsRootPath();
	}

	inline const std::filesystem::path& ProjectRoot()
	{
		return Filesystem::GetProjectPath();
	}

	inline const std::filesystem::path& ProjectAssetsRoot()
	{
		return Filesystem::GetProjectAssetsPath();
	}

	inline const std::filesystem::path& EngineRoot()
	{
		return Filesystem::GetEnginePath();
	}

	inline const std::filesystem::path& EngineAssetsRoot()
	{
		return Filesystem::GetEngineAssetsPath();
	}

	// Generated cooked/runtime output directories

	inline const std::filesystem::path& CookedAssetRoot()
	{
		return Filesystem::GetCookedAssetRootPath();
	}

	inline const std::filesystem::path& CookedShaderRoot()
	{
		return Filesystem::GetCookedShaderRootPath();
	}

	inline const std::filesystem::path& CookedShaderPackageRoot()
	{
		return Filesystem::GetCookedShaderPackageRootPath();
	}

	inline const std::filesystem::path& CookedTextureRoot()
	{
		return Filesystem::GetCookedTextureRootPath();
	}

	inline const std::filesystem::path& CookedSceneManifestRoot()
	{
		return Filesystem::GetCookedSceneManifestRootPath();
	}

	inline const std::filesystem::path& CookedMeshRoot()
	{
		return Filesystem::GetCookedMeshRootPath();
	}

	inline const std::filesystem::path& CookedMaterialRoot()
	{
		return Filesystem::GetCookedMaterialRootPath();
	}

	inline const std::filesystem::path& ShaderCacheRoot()
	{
		return Filesystem::GetShaderCacheRootPath();
	}

	inline const std::filesystem::path& ShaderDebugArtifactRoot()
	{
		return Filesystem::GetShaderDebugArtifactRootPath();
	}

	inline const std::filesystem::path& ShaderSymbolsOutputRoot()
	{
		return Filesystem::GetShaderSymbolsOutputPath();
	}

	// Source asset input directories

	inline const std::filesystem::path& TypedAssetRoot(AssetType type, PathRoot root = PathRoot::Any)
	{
		return Filesystem::GetTypedPath(type, root);
	}

	inline const std::filesystem::path& ShaderSourceRoot(PathRoot root = PathRoot::Any)
	{
		return Filesystem::GetShaderPath(root);
	}

	inline const std::filesystem::path& ShaderSymbolsSourceRoot(PathRoot root = PathRoot::Any)
	{
		return Filesystem::GetShaderSymbolsPath(root);
	}

	inline std::filesystem::path ProjectLevelsRoot()
	{
		return ProjectRoot() / "Levels";
	}

	// Process files
	inline std::filesystem::path ExecutablePath()
	{
		return Filesystem::GetExecutablePath();
	}

	inline std::array<std::filesystem::path, 3> ExecutableLookupCandidates(std::string_view executableFileName)
	{
		const std::filesystem::path executableDirectory = ExecutableDirectory();
		const std::filesystem::path executableName{std::string(executableFileName)};
		return {
		    executableDirectory / executableName,
		    executableDirectory.parent_path() / executableName,
		    executableDirectory.parent_path() / "Debug" / executableName};
	}

	// Log files
	inline std::string TimestampForFileName()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
		std::tm localTime{};
#if defined(_WIN32)
		localtime_s(&localTime, &nowTime);
#else
		localtime_r(&nowTime, &localTime);
#endif

		std::ostringstream stream;
		stream << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
		return stream.str();
	}

	inline std::filesystem::path TimestampedFileName(std::string_view stem, std::string_view extension)
	{
		std::string fileStem(stem.empty() ? "Sparkle" : stem);
		std::string fileExtension(extension.empty() ? ".log" : extension);
		if (!fileExtension.starts_with('.'))
		{
			fileExtension.insert(fileExtension.begin(), '.');
		}
		return fileStem + "_" + TimestampForFileName() + fileExtension;
	}

	inline std::filesystem::path DefaultLogFile(bool ensureParentExists = true, std::string_view executableStemOverride = {})
	{
		const std::filesystem::path logDirectory = ResolveLogsRoot(ensureParentExists);
		std::filesystem::path executableStem{std::string(executableStemOverride)};
		if (executableStem.empty())
		{
			executableStem = ExecutablePath().stem();
		}
		return logDirectory / TimestampedFileName(executableStem.string(), ".log");
	}

	inline std::filesystem::path LogFile(std::string_view configuredFile = {}, bool ensureParentExists = true)
	{
		if (configuredFile.empty())
		{
			return DefaultLogFile(ensureParentExists);
		}

		std::filesystem::path configuredPath{std::string(configuredFile)};
		if (configuredPath.empty())
		{
			return DefaultLogFile(ensureParentExists);
		}

		if (configuredPath.is_absolute())
		{
			configuredPath = configuredPath.filename();
		}

		return ResolveLogsRoot(ensureParentExists) / configuredPath;
	}

	// Cooked shader files

	inline const std::filesystem::path& CookedShaderRegistry()
	{
		return Filesystem::GetCookedShaderRegistryPath();
	}

	inline std::filesystem::path CookedShaderPackage(std::uint64_t packageKey)
	{
		return CookedShaderPackageRoot() / std::format("{:016X}.sshd", packageKey);
	}

	// Cooked scene/material/mesh/texture files

	inline const std::filesystem::path& SceneAssetRegistry()
	{
		return Filesystem::GetSceneAssetRegistryPath();
	}

	inline std::filesystem::path CookedSceneManifest(std::string_view sceneAssetId)
	{
		std::filesystem::path relativeScenePath{std::string(sceneAssetId)};
		relativeScenePath.replace_extension(".sscn");
		return CookedSceneManifestRoot() / relativeScenePath;
	}

	inline std::filesystem::path CookedSceneManifestRelative(const std::filesystem::path& relativeManifestPath)
	{
		return CookedSceneManifestRoot() / relativeManifestPath;
	}

	inline std::filesystem::path CookedMeshAsset(std::uint64_t meshAssetId)
	{
		return CookedMeshRoot() / std::format("{:016X}.smsh", meshAssetId);
	}

	inline std::filesystem::path CookedMaterialAsset(std::uint64_t materialAssetId)
	{
		return CookedMaterialRoot() / std::format("{:016X}.smat", materialAssetId);
	}

	inline std::filesystem::path CookedTextureAsset(std::uint64_t textureAssetId, std::string_view extension = {})
	{
		std::string normalizedExtension(extension.empty() ? ".stex" : extension);
		if (!normalizedExtension.starts_with('.'))
		{
			normalizedExtension.insert(normalizedExtension.begin(), '.');
		}
		return CookedTextureRoot() / std::format("{:016X}{}", textureAssetId, normalizedExtension);
	}

	// Shader cache/control files

	inline const std::filesystem::path& ShaderRecookSignal()
	{
		return Filesystem::GetShaderRecookSignalPath();
	}

	inline std::filesystem::path ShaderRecookSignal(const std::filesystem::path& shaderCacheRoot)
	{
		return Filesystem::BuildShaderRecookSignalPath(shaderCacheRoot);
	}
}  // namespace Paths
