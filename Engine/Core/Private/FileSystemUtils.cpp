#include "PCH.h"

#include "FileSystemUtils.h"

#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <array>
#include <cwctype>
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

namespace
{
	std::string GetExecutableStem()
	{
		return Strings::ToLowerCopy(Filesystem::GetExecutablePath().stem().string());
	}

	std::optional<std::filesystem::path> DiscoverWorkspaceProjectRoot()
	{
		const auto workspaceRoot = Filesystem::DiscoverWorkspaceRoot();
		if (!workspaceRoot)
		{
			return std::nullopt;
		}

		const std::filesystem::path projectsRoot = *workspaceRoot / "Projects";
		std::error_code ec;
		if (!std::filesystem::exists(projectsRoot, ec) || ec)
		{
			return std::nullopt;
		}

		const std::string executableStem = GetExecutableStem();
		if (executableStem.empty())
		{
			return std::nullopt;
		}
		for (const auto& entry : std::filesystem::directory_iterator(projectsRoot, ec))
		{
			if (ec)
			{
				break;
			}

			if (!entry.is_directory(ec) || ec)
			{
				ec.clear();
				continue;
			}

			const std::filesystem::path projectRoot = entry.path();
			if (!std::filesystem::exists(projectRoot / Filesystem::kProjectMarker, ec) || ec)
			{
				ec.clear();
				continue;
			}

			if (Strings::EqualsIgnoreCase(projectRoot.filename().string(), executableStem))
			{
				return Paths::Normalize(projectRoot);
			}
		}

		return std::nullopt;
	}

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
		std::filesystem::path sceneAssetRegistryPath;
		std::filesystem::path shaderCacheRootPath;
		std::filesystem::path shaderDebugArtifactRootPath;
		std::filesystem::path shaderRecookSignalPath;
		std::filesystem::path projectPath;
		std::filesystem::path projectAssetsPath;
		std::filesystem::path enginePath;
		std::filesystem::path engineAssetsPath;
		std::filesystem::path workingDirectory;
		std::filesystem::path executableDirectory;
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

		state.cookedAssetRootPath = Paths::Normalize(state.buildOutputRootPath / "Cooked" / cookedProjectName);
		state.cookedShaderRootPath = Paths::Normalize(state.cookedAssetRootPath / "Shaders");
		state.cookedShaderPackageRootPath = Paths::Normalize(state.cookedShaderRootPath / "Packages");
		state.cookedShaderRegistryPath = Paths::Normalize(state.cookedShaderRootPath / "ShaderPackageRegistry.sreg");
		state.cookedTextureRootPath = Paths::Normalize(state.cookedAssetRootPath / "Textures");
		state.cookedSceneManifestRootPath = Paths::Normalize(state.cookedAssetRootPath / "SceneManifests");
		state.cookedMeshRootPath = Paths::Normalize(state.cookedAssetRootPath / "Meshes");
		state.cookedMaterialRootPath = Paths::Normalize(state.cookedAssetRootPath / "Materials");
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
				else
				{
					SPDLOG_LOGGER_INFO(logger, "[--]       {}: (not configured)", paddedLabel);
				}
				return;
			}

			const bool exists = std::filesystem::exists(path, ec);
			if (exists)
			{
				SPDLOG_LOGGER_INFO(logger, "[OK]       {}: {}", paddedLabel, path.string());
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

		SPDLOG_LOGGER_INFO(logger, "========== Asset Paths Configuration ==========");
		logPath("Working Directory", state.workingDirectory, true);
		logPath("Executable Directory", state.executableDirectory, true);
		logPath("Workspace", state.workspacePath, true);
		logPath("Build Output Root", state.buildOutputRootPath, true);
		logPath("Logs Root", state.logsRootPath, true);
		logPath("Cooked Asset Root", state.cookedAssetRootPath, true);
		logPath("Cooked Shader Root", state.cookedShaderRootPath, true);
		logPath("Shader Cache Root", state.shaderCacheRootPath, true);
		logPath("Engine", state.enginePath, true);
		logPath("Engine Assets", state.engineAssetsPath, true);
		logPath("Project", state.projectPath, false);
		logPath("Project Assets", state.projectAssetsPath, false);
		logPath("Shader Symbols Output", state.shaderSymbolsOutputPath, false);
		SPDLOG_LOGGER_INFO(logger, "===============================================");
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

		state.workspacePath = Filesystem::ResolveWorkspaceRootPath();

		if (auto engineRoot = Filesystem::DiscoverEngineRoot())
		{
			state.enginePath = *engineRoot;
			state.engineAssetsPath = state.enginePath / "Assets";
		}

		if (auto projectRoot = Filesystem::DiscoverProjectRoot())
		{
			state.projectPath = *projectRoot;
			state.projectAssetsPath = state.projectPath / "Assets";
		}

		state.projectPath = Paths::Normalize(state.projectPath);
		state.projectAssetsPath = Paths::Normalize(state.projectAssetsPath);
		state.enginePath = Paths::Normalize(state.enginePath);
		state.engineAssetsPath = Paths::Normalize(state.engineAssetsPath);

		state.buildOutputRootPath = Filesystem::ResolveBuildOutputRootPath();
		state.logsRootPath = Filesystem::ResolveLogsRootPath();
		state.shaderCacheRootPath = Paths::Normalize(state.buildOutputRootPath / "Cache" / "Shaders");
		state.shaderDebugArtifactRootPath = Paths::Normalize(state.shaderCacheRootPath / "Debug");
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
}  // namespace

namespace Filesystem
{
	std::filesystem::path NormalizePath(const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return {};
		}

		auto normalized = path.is_relative() ? std::filesystem::absolute(path) : path;
		normalized.make_preferred();

		std::error_code ec;
		if (auto canonical = std::filesystem::weakly_canonical(normalized, ec); !ec)
		{
			return canonical;
		}
		return normalized;
	}

	std::wstring MakePathKey(const std::filesystem::path& path)
	{
		const std::filesystem::path normalizedPath = NormalizePath(path);
		if (normalizedPath.empty())
		{
			return {};
		}

		std::wstring key = normalizedPath.generic_wstring();
		std::transform(
		    key.begin(),
		    key.end(),
		    key.begin(),
		    [](wchar_t value)
		    {
			    return static_cast<wchar_t>(std::towlower(value));
		    });
		return key;
	}

	std::wstring GetLowercaseExtension(const std::filesystem::path& path)
	{
		std::wstring extension = path.extension().wstring();
		std::transform(
		    extension.begin(),
		    extension.end(),
		    extension.begin(),
		    [](wchar_t value)
		    {
			    return static_cast<wchar_t>(std::towlower(value));
		    });
		return extension;
	}

	const std::filesystem::path& GetWorkingDirectory()
	{
		return GetAssetPathState().workingDirectory;
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
			return NormalizePath(*workspaceRoot);
		}

		if (auto engineRoot = DiscoverEngineRoot())
		{
			return NormalizePath(engineRoot->parent_path());
		}

		std::error_code ec;
		const std::filesystem::path workingDirectory = std::filesystem::current_path(ec);
		if (!workingDirectory.empty() && !ec)
		{
			return NormalizePath(workingDirectory);
		}

		return NormalizePath(GetExecutableDirectory());
	}

	std::filesystem::path ResolveBuildOutputRootPath()
	{
		return NormalizePath(ResolveWorkspaceRootPath() / "build");
	}

	std::filesystem::path ResolveLogsRootPath()
	{
		return NormalizePath(ResolveWorkspaceRootPath() / "logs");
	}

	const std::filesystem::path& GetWorkspaceRootPath()
	{
		return GetAssetPathState().workspacePath;
	}

	const std::filesystem::path& GetBuildOutputRootPath()
	{
		return GetAssetPathState().buildOutputRootPath;
	}

	const std::filesystem::path& GetLogsRootPath()
	{
		return GetAssetPathState().logsRootPath;
	}

	const std::filesystem::path& GetCookedAssetRootPath()
	{
		return GetAssetPathState().cookedAssetRootPath;
	}

	const std::filesystem::path& GetCookedShaderRootPath()
	{
		return GetAssetPathState().cookedShaderRootPath;
	}

	const std::filesystem::path& GetCookedShaderPackageRootPath()
	{
		return GetAssetPathState().cookedShaderPackageRootPath;
	}

	const std::filesystem::path& GetCookedShaderRegistryPath()
	{
		return GetAssetPathState().cookedShaderRegistryPath;
	}

	const std::filesystem::path& GetCookedTextureRootPath()
	{
		return GetAssetPathState().cookedTextureRootPath;
	}

	const std::filesystem::path& GetCookedSceneManifestRootPath()
	{
		return GetAssetPathState().cookedSceneManifestRootPath;
	}

	const std::filesystem::path& GetCookedMeshRootPath()
	{
		return GetAssetPathState().cookedMeshRootPath;
	}

	const std::filesystem::path& GetCookedMaterialRootPath()
	{
		return GetAssetPathState().cookedMaterialRootPath;
	}

	const std::filesystem::path& GetSceneAssetRegistryPath()
	{
		return GetAssetPathState().sceneAssetRegistryPath;
	}

	const std::filesystem::path& GetShaderCacheRootPath()
	{
		return GetAssetPathState().shaderCacheRootPath;
	}

	const std::filesystem::path& GetShaderDebugArtifactRootPath()
	{
		return GetAssetPathState().shaderDebugArtifactRootPath;
	}

	const std::filesystem::path& GetShaderRecookSignalPath()
	{
		return GetAssetPathState().shaderRecookSignalPath;
	}

	std::filesystem::path BuildShaderRecookSignalPath(const std::filesystem::path& shaderCacheRootPath)
	{
		return NormalizePath(shaderCacheRootPath / "recook.signal");
	}

	const std::filesystem::path& GetProjectPath()
	{
		return GetAssetPathState().projectPath;
	}

	const std::filesystem::path& GetProjectAssetsPath()
	{
		return GetAssetPathState().projectAssetsPath;
	}

	const std::filesystem::path& GetEnginePath()
	{
		return GetAssetPathState().enginePath;
	}

	const std::filesystem::path& GetEngineAssetsPath()
	{
		return GetAssetPathState().engineAssetsPath;
	}

	void ConfigureProjectRoot(const std::filesystem::path& projectRoot)
	{
		AssetPathState& state = GetAssetPathState();
		state.projectPath = Paths::Normalize(projectRoot);
		state.projectAssetsPath = state.projectPath.empty() ? std::filesystem::path{} : Paths::Normalize(state.projectPath / "Assets");

		InitializeProjectOutputPaths(state);
		InitializeTypedPaths(state);
		InitializeOutputPaths(state);
		ValidatePaths(state);
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
			return NormalizePath(*fromExe);
		}

		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kWorkspaceMarker); fromCwd && !ec)
		{
			return NormalizePath(*fromCwd);
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverEngineRoot()
	{
		if (auto fromExe = FindAncestorWithMarker(GetExecutableDirectory(), kEngineMarker))
		{
			return NormalizePath(*fromExe);
		}

		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kEngineMarker); fromCwd && !ec)
		{
			return NormalizePath(*fromCwd);
		}

		if (auto workspace = DiscoverWorkspaceRoot())
		{
			auto enginePath = *workspace / "engine";
			if (std::filesystem::exists(enginePath / kEngineMarker, ec))
			{
				return NormalizePath(enginePath);
			}
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> DiscoverProjectRoot()
	{
		if (auto fromExe = FindAncestorWithMarker(GetExecutableDirectory(), kProjectMarker))
		{
			return NormalizePath(*fromExe);
		}

		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kProjectMarker); fromCwd && !ec)
		{
			return NormalizePath(*fromCwd);
		}

		if (auto fromWorkspace = DiscoverWorkspaceProjectRoot())
		{
			return NormalizePath(*fromWorkspace);
		}

		return std::nullopt;
	}

	const std::filesystem::path& GetTypedPath(AssetType type, PathRoot root) noexcept
	{
		AssetPathState& state = GetAssetPathState();
		const size_t idx = static_cast<size_t>(type);
		if (type == AssetType::Count || idx >= AssetPathState::kAssetTypeCount)
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
			return NormalizePath(*resolved);
		}

		return std::nullopt;
	}

	std::optional<std::filesystem::path> ResolveAssetPath(const std::filesystem::path& inputPath, AssetType type)
	{
		if (inputPath.empty())
		{
			return std::nullopt;
		}

		AssetPathState& state = GetAssetPathState();

		if (inputPath.is_absolute())
		{
			std::error_code ec;
			return std::filesystem::exists(inputPath, ec) ? std::make_optional(inputPath) : std::nullopt;
		}

		if (type == AssetType::Texture && Strings::EqualsIgnoreCase(inputPath.extension().string(), ".stex"))
		{
			if (auto result = TryResolveIn(state.cookedAssetRootPath, inputPath, type))
			{
				return result;
			}
		}

		if (auto result = TryResolveIn(state.projectAssetsPath, inputPath, type))
		{
			return result;
		}

		if (auto result = TryResolveIn(state.engineAssetsPath, inputPath, type))
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
		return GetAssetPathState().shaderSymbolsOutputPath;
	}
}  // namespace Filesystem
