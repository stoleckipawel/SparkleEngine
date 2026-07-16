#include "PCH.h"

#include "FileSystemUtils.h"

#include "Paths/FileSystemDiscovery.h"

#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

namespace Filesystem::Private
{
	struct AssetPathState
	{
		static constexpr size_t kAssetTypeCount = static_cast<size_t>(AssetType::Count);

		std::filesystem::path workspacePath;
		std::filesystem::path buildOutputRootPath;
		std::filesystem::path logsRootPath;
		std::filesystem::path cookedAssetRootPath;
		std::filesystem::path cookedShaderRootPath;
		std::filesystem::path cookedShaderPackageRootPath;
		std::filesystem::path cookedShaderRegistryPath;
		std::filesystem::path cookedTextureRootPath;
		std::filesystem::path cookedSceneManifestRootPath;
		std::filesystem::path cookedMeshRootPath;
		std::filesystem::path cookedMaterialRootPath;
		std::filesystem::path cookedSkeletonRootPath;
		std::filesystem::path cookedAnimationRootPath;
		std::filesystem::path sceneAssetRegistryPath;
		std::filesystem::path shaderCacheRootPath;
		std::filesystem::path shaderRecookSignalPath;
		std::filesystem::path projectPath;
		std::filesystem::path projectAssetsPath;
		std::filesystem::path enginePath;
		std::filesystem::path engineAssetsPath;
		std::filesystem::path workingDirectory;
		std::filesystem::path executableDirectory;
		bool packageRuntimeRoot = false;
		std::array<std::filesystem::path, kAssetTypeCount> projectTypedPaths{};
		std::array<std::filesystem::path, kAssetTypeCount> engineTypedPaths{};
		std::filesystem::path shaderSymbolsOutputPath;
		std::filesystem::path emptyPath;
	};

	void InitializeTypedPaths(AssetPathState& state)
	{
		state.projectTypedPaths.fill({});
		state.engineTypedPaths.fill({});

		auto buildTypedPaths =
		    [](const std::filesystem::path& root, std::array<std::filesystem::path, AssetPathState::kAssetTypeCount>& paths)
		{
			if (root.empty())
			{
				return;
			}

			for (size_t i = 0; i < AssetPathState::kAssetTypeCount; ++i)
			{
				const auto type = static_cast<AssetType>(i);
				const auto subdir = GetAssetSubdirectory(type);
				paths[i] = subdir.empty() ? root : root / subdir;
			}
		};

		buildTypedPaths(state.projectAssetsPath, state.projectTypedPaths);
		buildTypedPaths(state.engineAssetsPath, state.engineTypedPaths);
	}

	void InitializeProjectOutputPaths(AssetPathState& state)
	{
		const std::filesystem::path cookedProjectName =
		    !state.projectPath.empty() ? state.projectPath.filename() : std::filesystem::path("Shared");

		state.cookedAssetRootPath = state.packageRuntimeRoot ?
		                                Paths::Normalize(state.workspacePath / "Projects" / cookedProjectName / "Cooked") :
		                                Paths::Normalize(state.workspacePath / "artifacts" / "dev" / "projects" / cookedProjectName / "cooked");
		state.cookedShaderRootPath = Paths::Normalize(state.cookedAssetRootPath / "Shaders");
		state.cookedShaderPackageRootPath = Paths::Normalize(state.cookedShaderRootPath / "Packages");
		state.cookedShaderRegistryPath = Paths::Normalize(state.cookedShaderRootPath / "ShaderPackageRegistry.sreg");
		state.cookedTextureRootPath = Paths::Normalize(state.cookedAssetRootPath / "Textures");
		state.cookedSceneManifestRootPath = Paths::Normalize(state.cookedAssetRootPath / "SceneManifests");
		state.cookedMeshRootPath = Paths::Normalize(state.cookedAssetRootPath / "Meshes");
		state.cookedMaterialRootPath = Paths::Normalize(state.cookedAssetRootPath / "Materials");
		state.cookedSkeletonRootPath = Paths::Normalize(state.cookedAssetRootPath / "Skeletons");
		state.cookedAnimationRootPath = Paths::Normalize(state.cookedAssetRootPath / "Animations");
		state.sceneAssetRegistryPath = Paths::Normalize(state.cookedAssetRootPath / "SceneAssetRegistry.sreg");
		state.shaderSymbolsOutputPath = Paths::Normalize(state.buildOutputRootPath / "ShaderSymbols" / cookedProjectName);
	}

	void InitializeOutputPaths(AssetPathState& state)
	{
		std::error_code ec;
		std::filesystem::create_directories(state.buildOutputRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.logsRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.cookedAssetRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.cookedShaderPackageRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.cookedTextureRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.cookedSceneManifestRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.cookedMeshRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.cookedMaterialRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.cookedSkeletonRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.cookedAnimationRootPath, ec);
		ec.clear();
		std::filesystem::create_directories(state.shaderCacheRootPath, ec);

		if (!state.shaderSymbolsOutputPath.empty())
		{
			ec.clear();
			std::filesystem::create_directories(state.shaderSymbolsOutputPath, ec);
		}
	}

	void ValidatePaths(const AssetPathState& state)
	{
		std::error_code ec;
		auto logger = Logging::GetOrCreateLogger("Core");

		auto logPath = [&](const char* label, const std::filesystem::path& path, bool required)
		{
			constexpr int kLabelWidth = 24;
			const std::string paddedLabel = std::string(label) + std::string(kLabelWidth - strlen(label), ' ');

			if (path.empty())
			{
				if (required)
				{
					Diagnostics::Fail(logger, __FILE__, __LINE__, "[MISSING]  " + paddedLabel + ": (not configured)");
				}
				return;
			}

			const bool exists = std::filesystem::exists(path, ec);
			if (exists)
			{
				return;
			}

			if (required)
			{
				Diagnostics::Fail(logger, __FILE__, __LINE__, "[MISSING]  " + paddedLabel + ": " + path.string());
			}
			else
			{
				SPDLOG_LOGGER_WARN(logger, "[MISSING]  {}: {}", paddedLabel, path.string());
			}
		};

		logPath("Working Directory", state.workingDirectory, true);
		logPath("Executable Directory", state.executableDirectory, true);
		logPath("Workspace", state.workspacePath, true);
		logPath("Build Output Root", state.buildOutputRootPath, true);
		logPath("Logs Root", state.logsRootPath, true);
		logPath("Cooked Asset Root", state.cookedAssetRootPath, true);
		logPath("Cooked Shader Root", state.cookedShaderRootPath, true);
		logPath("Cooked Skeleton Root", state.cookedSkeletonRootPath, true);
		logPath("Cooked Animation Root", state.cookedAnimationRootPath, true);
		logPath("Shader Cache Root", state.shaderCacheRootPath, true);
		logPath("Engine", state.enginePath, !state.packageRuntimeRoot);
		logPath("Engine Assets", state.engineAssetsPath, !state.packageRuntimeRoot);
		logPath("Project", state.projectPath, false);
		logPath("Project Assets", state.projectAssetsPath, false);
		logPath("Shader Symbols Output", state.shaderSymbolsOutputPath, false);
	}

	std::optional<std::filesystem::path> TryResolveIn(
	    const std::filesystem::path& searchDir,
	    const std::filesystem::path& relativePath,
	    AssetType type)
	{
		if (searchDir.empty())
		{
			return std::nullopt;
		}

		std::error_code ec;

		if (const auto subdir = GetAssetSubdirectory(type); !subdir.empty())
		{
			auto candidate = searchDir / subdir / relativePath;
			if (std::filesystem::exists(candidate, ec))
			{
				return std::filesystem::weakly_canonical(candidate);
			}
		}

		auto candidate = searchDir / relativePath;
		if (std::filesystem::exists(candidate, ec))
		{
			return std::filesystem::weakly_canonical(candidate);
		}

		return std::nullopt;
	}

	AssetPathState CreateAssetPathState()
	{
		AssetPathState state;
		state.workingDirectory = std::filesystem::current_path();
		state.executableDirectory = Filesystem::GetExecutableDirectory();

		const std::optional<std::filesystem::path> packageRoot = DiscoverPackageRoot();
		state.packageRuntimeRoot = packageRoot.has_value();
		state.workspacePath = state.packageRuntimeRoot ? Paths::Normalize(*packageRoot) : Filesystem::ResolveWorkspaceRootPath();

		if (!state.packageRuntimeRoot)
		{
			if (auto engineRoot = Filesystem::DiscoverEngineRoot())
			{
				state.enginePath = *engineRoot;
				state.engineAssetsPath = state.enginePath / "Assets";
			}
		}

		if (state.packageRuntimeRoot)
		{
			if (auto projectRoot = DiscoverPackageProjectRoot(state.workspacePath))
			{
				state.projectPath = *projectRoot;
			}
		}
		else if (auto projectRoot = Filesystem::DiscoverProjectRoot())
		{
			state.projectPath = *projectRoot;
			state.projectAssetsPath = state.projectPath / "Assets";
		}

		state.projectPath = Paths::Normalize(state.projectPath);
		state.projectAssetsPath = Paths::Normalize(state.projectAssetsPath);
		state.enginePath = Paths::Normalize(state.enginePath);
		state.engineAssetsPath = Paths::Normalize(state.engineAssetsPath);

		state.buildOutputRootPath = state.packageRuntimeRoot ? Paths::Normalize(state.workspacePath / "build") : Filesystem::ResolveBuildOutputRootPath();
		state.logsRootPath = state.packageRuntimeRoot ? Paths::Normalize(state.workspacePath / "logs") : Filesystem::ResolveLogsRootPath();
		state.shaderCacheRootPath = Paths::Normalize(state.buildOutputRootPath / "Cache" / "Shaders");
		state.shaderRecookSignalPath = Filesystem::BuildShaderRecookSignalPath(state.shaderCacheRootPath);
		InitializeProjectOutputPaths(state);

		InitializeTypedPaths(state);
		InitializeOutputPaths(state);
		ValidatePaths(state);
		return state;
	}

	AssetPathState& GetAssetPathState()
	{
		static AssetPathState state = CreateAssetPathState();
		return state;
	}
}  // namespace Filesystem::Private

namespace Filesystem
{
	const std::filesystem::path& GetWorkingDirectory()
	{
		return Private::GetAssetPathState().workingDirectory;
	}

	std::filesystem::path GetExecutablePath()
	{
#if defined(_WIN32)
		wchar_t buffer[MAX_PATH];
		const DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		if (len > 0 && len < MAX_PATH)
		{
			return std::filesystem::path(buffer);
		}
#endif
		return {};
	}

	std::filesystem::path GetExecutableDirectory()
	{
		const std::filesystem::path executablePath = GetExecutablePath();
		if (!executablePath.empty())
		{
			return executablePath.parent_path();
		}
		return std::filesystem::current_path();
	}

	std::filesystem::path ResolveWorkspaceRootPath()
	{
		if (auto workspaceRoot = DiscoverWorkspaceRoot())
		{
			return Paths::Normalize(*workspaceRoot);
		}

		if (auto engineRoot = DiscoverEngineRoot())
		{
			return Paths::Normalize(engineRoot->parent_path());
		}

		std::error_code ec;
		const std::filesystem::path workingDirectory = std::filesystem::current_path(ec);
		if (!workingDirectory.empty() && !ec)
		{
			return Paths::Normalize(workingDirectory);
		}

		return Paths::Normalize(GetExecutableDirectory());
	}

	std::filesystem::path ResolveBuildOutputRootPath()
	{
		return Paths::Normalize(ResolveWorkspaceRootPath() / "build");
	}

	std::filesystem::path ResolveLogsRootPath()
	{
		return Paths::Normalize(ResolveWorkspaceRootPath() / "logs");
	}

	const std::filesystem::path& GetWorkspaceRootPath()
	{
		return Private::GetAssetPathState().workspacePath;
	}

	const std::filesystem::path& GetBuildOutputRootPath()
	{
		return Private::GetAssetPathState().buildOutputRootPath;
	}

	const std::filesystem::path& GetLogsRootPath()
	{
		return Private::GetAssetPathState().logsRootPath;
	}

	const std::filesystem::path& GetCookedAssetRootPath()
	{
		return Private::GetAssetPathState().cookedAssetRootPath;
	}

	const std::filesystem::path& GetCookedShaderRootPath()
	{
		return Private::GetAssetPathState().cookedShaderRootPath;
	}

	const std::filesystem::path& GetCookedShaderPackageRootPath()
	{
		return Private::GetAssetPathState().cookedShaderPackageRootPath;
	}

	const std::filesystem::path& GetCookedShaderRegistryPath()
	{
		return Private::GetAssetPathState().cookedShaderRegistryPath;
	}

	const std::filesystem::path& GetCookedTextureRootPath()
	{
		return Private::GetAssetPathState().cookedTextureRootPath;
	}

	const std::filesystem::path& GetCookedSceneManifestRootPath()
	{
		return Private::GetAssetPathState().cookedSceneManifestRootPath;
	}

	const std::filesystem::path& GetCookedMeshRootPath()
	{
		return Private::GetAssetPathState().cookedMeshRootPath;
	}

	const std::filesystem::path& GetCookedMaterialRootPath()
	{
		return Private::GetAssetPathState().cookedMaterialRootPath;
	}

	const std::filesystem::path& GetCookedSkeletonRootPath()
	{
		return Private::GetAssetPathState().cookedSkeletonRootPath;
	}

	const std::filesystem::path& GetCookedAnimationRootPath()
	{
		return Private::GetAssetPathState().cookedAnimationRootPath;
	}

	const std::filesystem::path& GetSceneAssetRegistryPath()
	{
		return Private::GetAssetPathState().sceneAssetRegistryPath;
	}

	const std::filesystem::path& GetShaderCacheRootPath()
	{
		return Private::GetAssetPathState().shaderCacheRootPath;
	}

	const std::filesystem::path& GetShaderRecookSignalPath()
	{
		return Private::GetAssetPathState().shaderRecookSignalPath;
	}

	std::filesystem::path BuildShaderRecookSignalPath(const std::filesystem::path& shaderCacheRootPath)
	{
		return Paths::Normalize(shaderCacheRootPath / "recook.signal");
	}

	const std::filesystem::path& GetProjectPath()
	{
		return Private::GetAssetPathState().projectPath;
	}

	const std::filesystem::path& GetProjectAssetsPath()
	{
		return Private::GetAssetPathState().projectAssetsPath;
	}

	const std::filesystem::path& GetEnginePath()
	{
		return Private::GetAssetPathState().enginePath;
	}

	const std::filesystem::path& GetEngineAssetsPath()
	{
		return Private::GetAssetPathState().engineAssetsPath;
	}

	void ConfigureProjectRoot(const std::filesystem::path& projectRoot)
	{
		Private::AssetPathState& state = Private::GetAssetPathState();
		state.projectPath = Paths::Normalize(projectRoot);
		state.projectAssetsPath = state.projectPath.empty() ? std::filesystem::path{} : Paths::Normalize(state.projectPath / "Assets");

		Private::InitializeProjectOutputPaths(state);
		Private::InitializeTypedPaths(state);
		Private::InitializeOutputPaths(state);
		Private::ValidatePaths(state);
	}

	std::optional<std::filesystem::path> FindAncestorWithMarker(
	    const std::filesystem::path& startDir,
	    std::string_view markerFileName,
	    uint32_t maxDepth)
	{
		if (startDir.empty() || markerFileName.empty())
		{
			return std::nullopt;
		}

		std::error_code ec;
		auto currentDir = std::filesystem::weakly_canonical(startDir, ec);
		if (ec)
		{
			currentDir = startDir;
		}

		for (uint32_t depth = 0; depth < maxDepth && !currentDir.empty(); ++depth)
		{
			if (std::filesystem::exists(currentDir / markerFileName, ec))
			{
				return currentDir;
			}

			auto parentDir = currentDir.parent_path();
			if (parentDir == currentDir)
			{
				break;
			}
			currentDir = std::move(parentDir);
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverWorkspaceRoot()
	{
		if (auto fromExe = FindAncestorWithMarker(GetExecutableDirectory(), kWorkspaceMarker))
		{
			return Paths::Normalize(*fromExe);
		}

		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kWorkspaceMarker); fromCwd && !ec)
		{
			return Paths::Normalize(*fromCwd);
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverEngineRoot()
	{
		if (auto fromExe = FindAncestorWithMarker(GetExecutableDirectory(), kEngineMarker))
		{
			return Paths::Normalize(*fromExe);
		}

		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kEngineMarker); fromCwd && !ec)
		{
			return Paths::Normalize(*fromCwd);
		}

		if (auto workspace = DiscoverWorkspaceRoot())
		{
			auto enginePath = *workspace / "engine";
			if (std::filesystem::exists(enginePath / kEngineMarker, ec))
			{
				return Paths::Normalize(enginePath);
			}
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverProjectRoot()
	{
		if (auto fromExe = FindAncestorWithMarker(GetExecutableDirectory(), kProjectMarker))
		{
			return Paths::Normalize(*fromExe);
		}

		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kProjectMarker); fromCwd && !ec)
		{
			return Paths::Normalize(*fromCwd);
		}

		if (auto fromWorkspace = Private::DiscoverWorkspaceProjectRoot())
		{
			return Paths::Normalize(*fromWorkspace);
		}

		return std::nullopt;
	}

	const std::filesystem::path& GetTypedPath(AssetType type, PathRoot root) noexcept
	{
		Private::AssetPathState& state = Private::GetAssetPathState();
		const size_t idx = static_cast<size_t>(type);
		if (type == AssetType::Count || idx >= Private::AssetPathState::kAssetTypeCount)
		{
			return state.emptyPath;
		}

		switch (root)
		{
			case PathRoot::Project:
				return state.projectTypedPaths[idx];

			case PathRoot::Engine:
				return state.engineTypedPaths[idx];

			case PathRoot::Any:
			default:
			{
				const auto& projectPath = state.projectTypedPaths[idx];
				return !projectPath.empty() ? projectPath : state.engineTypedPaths[idx];
			}
		}
	}

	const std::filesystem::path& GetShaderPath(PathRoot root) noexcept
	{
		return GetTypedPath(AssetType::Shader, root);
	}

	std::optional<std::filesystem::path> ResolveAssetPathNormalized(const std::filesystem::path& inputPath, AssetType type)
	{
		if (auto resolved = ResolveAssetPath(inputPath, type))
		{
			return Paths::Normalize(*resolved);
		}

		return std::nullopt;
	}

	void AppendNormalizedAssetPaths(
	    std::span<const std::filesystem::path> inputPaths,
	    AssetType type,
	    std::vector<std::filesystem::path>& destination)
	{
		for (const std::filesystem::path& inputPath : inputPaths)
		{
			std::filesystem::path normalizedPath = ResolveAssetPathNormalized(inputPath, type).value_or(Paths::Normalize(inputPath));
			if (!normalizedPath.empty())
			{
				destination.push_back(std::move(normalizedPath));
			}
		}
	}

	std::optional<std::filesystem::path> ResolveAssetPath(const std::filesystem::path& inputPath, AssetType type)
	{
		if (inputPath.empty())
		{
			return std::nullopt;
		}

		Private::AssetPathState& state = Private::GetAssetPathState();

		if (inputPath.is_absolute())
		{
			std::error_code ec;
			return std::filesystem::exists(inputPath, ec) ? std::make_optional(inputPath) : std::nullopt;
		}

		if (type == AssetType::Texture && Strings::EqualsIgnoreCase(inputPath.extension().string(), ".stex"))
		{
			if (auto result = Private::TryResolveIn(state.cookedAssetRootPath, inputPath, type))
			{
				return result;
			}
		}

		if (auto result = Private::TryResolveIn(state.projectAssetsPath, inputPath, type))
		{
			return result;
		}

		if (auto result = Private::TryResolveIn(state.engineAssetsPath, inputPath, type))
		{
			return result;
		}

		return std::nullopt;
	}

	std::filesystem::path ResolveAssetPathValidated(const std::filesystem::path& inputPath, AssetType type)
	{
		if (auto resolved = ResolveAssetPath(inputPath, type))
		{
			return *resolved;
		}

		Diagnostics::Fail(
		    Logging::GetOrCreateLogger("Core.FileSystem"),
		    __FILE__,
		    __LINE__,
		    std::string(GetAssetTypeName(type)) + " asset not found: " + inputPath.string());
		return {};
	}

	const std::filesystem::path& GetShaderSymbolsOutputPath()
	{
		return Private::GetAssetPathState().shaderSymbolsOutputPath;
	}
}  // namespace Filesystem
