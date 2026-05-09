#include "PCH.h"

#include "Cooking/CookedSceneCooker.h"

#include "MaterialCooker.h"
#include "MeshCooker.h"

CookedSceneBuild CookedSceneCooker::Cook(
    const std::filesystem::path& sourceScenePath,
    const SourceImportResult& importResult) const
{
	CookedSceneBuild build;
	if (!importResult.IsValid())
	{
		build.errorMessage = "Scene import result is not valid";
		return build;
	}

	if (!SceneCooker::ResolveSceneAsset(sourceScenePath, build.sceneAssetId, build.sceneManifestPath, build.errorMessage))
	{
		return build;
	}

	MeshCooker::BuildMeshAssets(importResult, build.sceneAssetId, build.meshAssets, build.meshAssetReferences);
	if (!MaterialCooker::BuildMaterialAssets(
	        importResult,
	        build.sceneAssetId,
	        build.materialAssets,
	        build.materialAssetReferences,
	        build.errorMessage))
	{
		return build;
	}

	if (!SceneCooker::BuildManifest(importResult, build, build.errorMessage))
	{
		return build;
	}

	if (!MeshCooker::WriteMeshAssets(build.meshAssets, build.errorMessage) ||
	    !MaterialCooker::WriteMaterialAssets(build.materialAssets, build.errorMessage) ||
	    !SceneCooker::WriteSceneManifestAndRegistry(build, build.errorMessage))
	{
		return build;
	}

	return build;
}

bool CookedSceneCooker::CollectTextureCookRequests(
    const SourceImportResult& importResult,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage) const
{
	return MaterialCooker::CollectTextureCookRequests(importResult, outRequests, outErrorMessage);
}
