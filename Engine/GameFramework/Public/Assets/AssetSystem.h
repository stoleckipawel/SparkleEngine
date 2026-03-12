// ============================================================================
// AssetSystem.h
// Unified asset path resolution service with marker-based discovery.
// ----------------------------------------------------------------------------
#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include "Assets/AssetTypes.h"
#include "Paths/PathRoot.h"

#include <array>
#include <filesystem>
#include <optional>

class SPARKLE_ENGINE_API AssetSystem final
{
  public:
	AssetSystem();
	~AssetSystem();

	AssetSystem(const AssetSystem&) = delete;
	AssetSystem& operator=(const AssetSystem&) = delete;
	AssetSystem(AssetSystem&&) = delete;
	AssetSystem& operator=(AssetSystem&&) = delete;

	// =========================================================================
	// Root Path Accessors
	// =========================================================================

	const std::filesystem::path& GetProjectPath() const noexcept { return m_projectPath; }
	const std::filesystem::path& GetProjectAssetsPath() const noexcept { return m_projectAssetsPath; }
	const std::filesystem::path& GetEnginePath() const noexcept { return m_enginePath; }
	const std::filesystem::path& GetEngineAssetsPath() const noexcept { return m_engineAssetsPath; }

	const std::filesystem::path& GetWorkingDirectory() const noexcept { return m_workingDirectory; }
	const std::filesystem::path& GetExecutableDirectory() const noexcept { return m_executableDirectory; }

	// =========================================================================
	// Path Accessors
	// =========================================================================

	// Returns the directory path for a specific asset type.
	// When root is Any: returns Project path if available, otherwise Engine.
	const std::filesystem::path& GetTypedPath(AssetType type, PathRoot root = PathRoot::Any) const noexcept;

	const std::filesystem::path& GetShaderPath(PathRoot root = PathRoot::Any) const noexcept;
	const std::filesystem::path& GetShaderSymbolsPath(PathRoot root = PathRoot::Any) const noexcept;
	const std::filesystem::path& GetTexturePath(PathRoot root = PathRoot::Any) const noexcept;
	const std::filesystem::path& GetMeshPath(PathRoot root = PathRoot::Any) const noexcept;
	const std::filesystem::path& GetMaterialPath(PathRoot root = PathRoot::Any) const noexcept;
	const std::filesystem::path& GetScenePath(PathRoot root = PathRoot::Any) const noexcept;
	const std::filesystem::path& GetAudioPath(PathRoot root = PathRoot::Any) const noexcept;
	const std::filesystem::path& GetFontPath(PathRoot root = PathRoot::Any) const noexcept;

	// =========================================================================
	// Path Resolution
	// =========================================================================

	// Resolves a virtual path to an absolute physical path.
	// Searches Project first, then Engine. Returns nullopt if not found.
	std::optional<std::filesystem::path> ResolvePath(const std::filesystem::path& virtualPath, AssetType type) const;

	// Resolves a virtual path to an absolute physical path.
	// Fatals if the asset cannot be found. Use when the asset is required.
	std::filesystem::path ResolvePathValidated(const std::filesystem::path& virtualPath, AssetType type) const;

	// =========================================================================
	// Output Paths
	// =========================================================================

	const std::filesystem::path& GetShaderSymbolsOutputPath() const noexcept { return m_shaderSymbolsOutputPath; }

	// =========================================================================
	// Queries
	// =========================================================================

	bool HasProjectAssets() const noexcept { return !m_projectAssetsPath.empty(); }
	bool HasEngineAssets() const noexcept { return !m_engineAssetsPath.empty(); }

  private:
	void DiscoverPaths();
	void InitializeTypedPaths();
	void InitializeOutputPaths();
	void ValidatePaths();

	std::optional<std::filesystem::path> TryResolveIn(
	    const std::filesystem::path& searchDir,
	    const std::filesystem::path& relativePath,
	    AssetType type) const;

	static constexpr size_t kAssetTypeCount = static_cast<size_t>(AssetType::Count);

	// Discovered root paths
	std::filesystem::path m_projectPath;
	std::filesystem::path m_projectAssetsPath;
	std::filesystem::path m_enginePath;
	std::filesystem::path m_engineAssetsPath;
	std::filesystem::path m_workingDirectory;
	std::filesystem::path m_executableDirectory;

	// Cached typed paths for fast lookup
	std::array<std::filesystem::path, kAssetTypeCount> m_projectTypedPaths{};
	std::array<std::filesystem::path, kAssetTypeCount> m_engineTypedPaths{};

	// Output directories
	std::filesystem::path m_shaderSymbolsOutputPath;

	inline static const std::filesystem::path s_emptyPath{};
};
