#include "PCH.h"

#include "AssetPathState.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Paths/FileSystemDiscovery.h"

#include <array>
#include <cstring>
#include <string>
#include <system_error>

namespace Filesystem::Private
{
	namespace
	{
		void RebuildTypedPaths(AssetPathState& state)
		{
			const auto buildPaths = [](const std::filesystem::path& root, auto& paths)
			{
				paths.fill({});
				if (root.empty())
				{
					return;
				}

				for (std::size_t index = 0; index < AssetPathState::AssetTypeCount; ++index)
				{
					const std::string_view subdirectory = GetAssetSubdirectory(static_cast<AssetType>(index));
					paths[index] = subdirectory.empty() ? root : root / subdirectory;
				}
			};

			buildPaths(state.projectAssetsPath, state.projectTypedPaths);
			buildPaths(state.engineAssetsPath, state.engineTypedPaths);
		}

		void RebuildCookedPaths(AssetPathState& state)
		{
			const std::filesystem::path projectName =
			    state.projectPath.empty() ? std::filesystem::path("Shared") : state.projectPath.filename();
			state.cookedAssetRootPath = state.packageRuntimeRoot ?
			                                Paths::Normalize(state.workspacePath / "Projects" / projectName / "Cooked") :
			                                Paths::Normalize(state.workspacePath / "artifacts" / "dev" / "projects" / projectName / "cooked");
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
			state.shaderSymbolsOutputPath = Paths::Normalize(state.buildOutputRootPath / "ShaderSymbols" / projectName);
		}

		void MaterializeOutputDirectories(const AssetPathState& state)
		{
			const std::array outputDirectories = {
			    &state.buildOutputRootPath,
			    &state.logsRootPath,
			    &state.cookedAssetRootPath,
			    &state.cookedShaderPackageRootPath,
			    &state.cookedTextureRootPath,
			    &state.cookedSceneManifestRootPath,
			    &state.cookedMeshRootPath,
			    &state.cookedMaterialRootPath,
			    &state.cookedSkeletonRootPath,
			    &state.cookedAnimationRootPath,
			    &state.shaderCacheRootPath,
			    &state.shaderSymbolsOutputPath,
			};

			for (const std::filesystem::path* outputDirectory : outputDirectories)
			{
				if (outputDirectory->empty())
				{
					continue;
				}
				std::error_code errorCode;
				std::filesystem::create_directories(*outputDirectory, errorCode);
			}
		}

		void ValidatePath(
		    const std::shared_ptr<spdlog::logger>& logger,
		    const char* label,
		    const std::filesystem::path& path,
		    bool required)
		{
			constexpr std::size_t LabelWidth = 24;
			const std::size_t labelLength = std::strlen(label);
			const std::string paddedLabel = std::string(label) + std::string(LabelWidth - labelLength, ' ');
			if (path.empty())
			{
				if (required)
				{
					Diagnostics::Fail(logger, __FILE__, __LINE__, "[MISSING]  " + paddedLabel + ": (not configured)");
				}
				return;
			}

			std::error_code errorCode;
			if (std::filesystem::exists(path, errorCode))
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
		}

		void ValidateConfiguredPaths(const AssetPathState& state)
		{
			const auto logger = Logging::GetOrCreateLogger("Core");
			ValidatePath(logger, "Working Directory", state.workingDirectory, true);
			ValidatePath(logger, "Executable Directory", state.executableDirectory, true);
			ValidatePath(logger, "Workspace", state.workspacePath, true);
			ValidatePath(logger, "Build Output Root", state.buildOutputRootPath, true);
			ValidatePath(logger, "Logs Root", state.logsRootPath, true);
			ValidatePath(logger, "Cooked Asset Root", state.cookedAssetRootPath, true);
			ValidatePath(logger, "Cooked Shader Root", state.cookedShaderRootPath, true);
			ValidatePath(logger, "Cooked Skeleton Root", state.cookedSkeletonRootPath, true);
			ValidatePath(logger, "Cooked Animation Root", state.cookedAnimationRootPath, true);
			ValidatePath(logger, "Shader Cache Root", state.shaderCacheRootPath, true);
			ValidatePath(logger, "Engine", state.enginePath, !state.packageRuntimeRoot);
			ValidatePath(logger, "Engine Assets", state.engineAssetsPath, !state.packageRuntimeRoot);
			ValidatePath(logger, "Project", state.projectPath, false);
			ValidatePath(logger, "Project Assets", state.projectAssetsPath, false);
			ValidatePath(logger, "Shader Symbols Output", state.shaderSymbolsOutputPath, false);
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
				if (const auto engineRoot = Filesystem::DiscoverEngineRoot())
				{
					state.enginePath = *engineRoot;
					state.engineAssetsPath = state.enginePath / "Assets";
				}
			}

			if (state.packageRuntimeRoot)
			{
				if (const auto projectRoot = DiscoverPackageProjectRoot(state.workspacePath))
				{
					state.projectPath = *projectRoot;
				}
			}
			else if (const auto projectRoot = Filesystem::DiscoverProjectRoot())
			{
				state.projectPath = *projectRoot;
				state.projectAssetsPath = state.projectPath / "Assets";
			}

			state.projectPath = Paths::Normalize(state.projectPath);
			state.projectAssetsPath = Paths::Normalize(state.projectAssetsPath);
			state.enginePath = Paths::Normalize(state.enginePath);
			state.engineAssetsPath = Paths::Normalize(state.engineAssetsPath);
			state.buildOutputRootPath = state.packageRuntimeRoot ?
			                                Paths::Normalize(state.workspacePath / "build") :
			                                Filesystem::ResolveBuildOutputRootPath();
			state.logsRootPath = state.packageRuntimeRoot ?
			                         Paths::Normalize(state.workspacePath / "logs") :
			                         Filesystem::ResolveLogsRootPath();
			state.shaderCacheRootPath = Paths::Normalize(state.buildOutputRootPath / "Cache" / "Shaders");
			state.shaderRecookSignalPath = Filesystem::BuildShaderRecookSignalPath(state.shaderCacheRootPath);
			RebuildProjectPaths(state);
			return state;
		}
	}

	void RebuildProjectPaths(AssetPathState& state)
	{
		RebuildCookedPaths(state);
		RebuildTypedPaths(state);
		MaterializeOutputDirectories(state);
		ValidateConfiguredPaths(state);
	}

	AssetPathState& GetAssetPathState()
	{
		static AssetPathState state = CreateAssetPathState();
		return state;
	}
}
