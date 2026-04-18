#pragma once

#include "Assets/Import/SceneImportResult.h"
#include "GameFramework/Public/Assets/Cooked/CookedMaterialAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedMeshAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"
#include "TextureCookRequestList.h"

#include <filesystem>
#include <string>
#include <vector>

namespace Engine::AssetAuthoring
{
	struct CookedMeshAssetBuild
	{
		Engine::Assets::CookedAssetId assetId = Engine::Assets::InvalidCookedAssetId;
		std::vector<Engine::Assets::CookedMeshVertex> vertices;
		std::vector<std::uint32_t> indices;
	};

	struct CookedMaterialAssetBuild
	{
		Engine::Assets::CookedAssetId assetId = Engine::Assets::InvalidCookedAssetId;
		Engine::Assets::CookedMaterialAssetHeader header;
		std::string name;
		std::vector<Engine::Assets::CookedTextureReference> textureReferences;
	};

	struct CookedSceneBuild
	{
		std::string sceneAssetId;
		std::filesystem::path sceneManifestPath;
		Engine::Assets::CookedSceneManifestHeader manifestHeader;
		std::vector<Engine::Assets::CookedSceneMeshAssetRef> meshAssetReferences;
		std::vector<Engine::Assets::CookedSceneMaterialAssetRef> materialAssetReferences;
		std::vector<Engine::Assets::CookedSceneInstanceRecord> instances;
		std::vector<CookedMeshAssetBuild> meshAssets;
		std::vector<CookedMaterialAssetBuild> materialAssets;
		std::string errorMessage;

		bool Succeeded() const noexcept { return errorMessage.empty(); }
	};

	class CookedSceneCooker final
	{
	  public:
		CookedSceneBuild Cook(const std::filesystem::path& sourceScenePath, const SceneImportResult& importResult) const;
		bool CollectTextureCookRequests(
		    const SceneImportResult& importResult,
		    std::vector<TextureCookRequest>& outRequests,
		    std::string& outErrorMessage) const;

	  private:
		static bool ResolveSourceScenePath(
		    const std::filesystem::path& sourceScenePath,
		    std::filesystem::path& outResolvedPath,
		    std::string& outErrorMessage);
		static bool BuildSceneAssetId(
		    const std::filesystem::path& resolvedSourceScenePath,
		    std::string& outSceneAssetId,
		    std::string& outErrorMessage);
		static Engine::Assets::CookedAssetId BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept;
		static Engine::Assets::CookedAssetId BuildMaterialAssetId(std::string_view sceneAssetId, std::size_t materialIndex) noexcept;
		static std::filesystem::path BuildSceneManifestPath(std::string_view sceneAssetId);
		static std::filesystem::path BuildMeshAssetPath(Engine::Assets::CookedAssetId meshAssetId);
		static std::filesystem::path BuildMaterialAssetPath(Engine::Assets::CookedAssetId materialAssetId);
		static Engine::Assets::CookedAlphaMode TranslateAlphaMode(AlphaMode alphaMode) noexcept;
		static void BuildMeshAssets(const SceneImportResult& importResult, std::string_view sceneAssetId, CookedSceneBuild& outBuild);
		static bool BuildMaterialAssets(
		    const SceneImportResult& importResult,
		    std::string_view sceneAssetId,
		    CookedSceneBuild& outBuild,
		    std::string& outErrorMessage);
		static bool BuildManifest(const SceneImportResult& importResult, CookedSceneBuild& outBuild, std::string& outErrorMessage);
		static bool UpdateSceneAssetRegistry(const CookedSceneBuild& build, std::string& outErrorMessage);
		static bool WriteBuildOutputs(const CookedSceneBuild& build, std::string& outErrorMessage);
	};
}