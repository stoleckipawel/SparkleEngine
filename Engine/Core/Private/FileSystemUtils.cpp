#include "PCH.h"

#include "FileSystemUtils.h"

#include "Core/Public/Diagnostics/Log.h"

#include <array>

namespace
{
	struct AssetPathState
	{
		static constexpr size_t kAssetTypeCount = static_cast<size_t>(AssetType::Count);

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
		auto buildTypedPaths = [](const std::filesystem::path& root, std::array<std::filesystem::path, AssetPathState::kAssetTypeCount>& paths)
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

	void InitializeOutputPaths(AssetPathState& state)
	{
		const auto& outputRoot = !state.projectAssetsPath.empty() ? state.projectAssetsPath : state.engineAssetsPath;

		if (!outputRoot.empty())
		{
			state.shaderSymbolsOutputPath = outputRoot / GetAssetSubdirectory(AssetType::ShaderSymbols);

			std::error_code ec;
			std::filesystem::create_directories(state.shaderSymbolsOutputPath, ec);
		}
	}

	void ValidatePaths(const AssetPathState& state)
	{
		std::error_code ec;

		auto logPath = [&](const char* label, const std::filesystem::path& path, bool required)
		{
			constexpr int kLabelWidth = 24;
			const std::string paddedLabel = std::string(label) + std::string(kLabelWidth - strlen(label), ' ');

			if (path.empty())
			{
				if (required)
				{
					LOG_FATAL("[MISSING]  " + paddedLabel + ": (not configured)");
				}
				else
				{
					LOG_INFO("[--]       " + paddedLabel + ": (not configured)");
				}
				return;
			}

			const bool exists = std::filesystem::exists(path, ec);
			if (exists)
			{
				LOG_INFO("[OK]       " + paddedLabel + ": " + path.string());
				return;
			}

			if (required)
			{
				LOG_FATAL("[MISSING]  " + paddedLabel + ": " + path.string());
			}
			else
			{
				LOG_WARNING("[MISSING]  " + paddedLabel + ": " + path.string());
			}
		};

		LOG_INFO("========== Asset Paths Configuration ==========");
		logPath("Working Directory", state.workingDirectory, true);
		logPath("Executable Directory", state.executableDirectory, true);
		logPath("Engine", state.enginePath, true);
		logPath("Engine Assets", state.engineAssetsPath, true);
		logPath("Project", state.projectPath, false);
		logPath("Project Assets", state.projectAssetsPath, false);
		logPath("Shader Symbols Output", state.shaderSymbolsOutputPath, false);
		LOG_INFO("===============================================");
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

		state.projectPath = Filesystem::NormalizePath(state.projectPath);
		state.projectAssetsPath = Filesystem::NormalizePath(state.projectAssetsPath);
		state.enginePath = Filesystem::NormalizePath(state.enginePath);
		state.engineAssetsPath = Filesystem::NormalizePath(state.engineAssetsPath);

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

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif

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

	const std::filesystem::path& GetWorkingDirectory()
	{
		return GetAssetPathState().workingDirectory;
	}

	std::filesystem::path GetExecutableDirectory()
	{
#if defined(_WIN32)
		wchar_t buffer[MAX_PATH];
		const DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		if (len > 0 && len < MAX_PATH)
		{
			return std::filesystem::path(buffer).parent_path();
		}
#endif
		return std::filesystem::current_path();
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
		std::error_code ec;
		if (auto fromCwd = FindAncestorWithMarker(std::filesystem::current_path(ec), kProjectMarker); fromCwd && !ec)
		{
			return NormalizePath(*fromCwd);
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

	const std::filesystem::path& GetShaderSymbolsPath(PathRoot root) noexcept
	{
		return GetTypedPath(AssetType::ShaderSymbols, root);
	}

	const std::filesystem::path& GetTexturePath(PathRoot root) noexcept
	{
		return GetTypedPath(AssetType::Texture, root);
	}

	const std::filesystem::path& GetMeshPath(PathRoot root) noexcept
	{
		return GetTypedPath(AssetType::Mesh, root);
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

		LOG_FATAL(std::string(GetAssetTypeName(type)) + " asset not found: " + inputPath.string());
		return {};
	}

	const std::filesystem::path& GetShaderSymbolsOutputPath()
	{
		return GetAssetPathState().shaderSymbolsOutputPath;
	}

	bool HasProjectAssets() noexcept
	{
		return !GetAssetPathState().projectAssetsPath.empty();
	}

		bool HasEngineAssets() noexcept
		{
			return !GetAssetPathState().engineAssetsPath.empty();
		}
	}  // namespace Filesystem
