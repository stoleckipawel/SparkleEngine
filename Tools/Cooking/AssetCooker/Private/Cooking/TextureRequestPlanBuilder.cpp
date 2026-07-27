#include "TextureRequestPlanBuilder.h"

#include "DefaultTextureCookRequestBuilder.h"
#include "ImportedSceneCooker.h"
#include "MaterialCooker.h"
#include "SourceSceneImporter.h"
#include "TextureCookRequestList.h"

#include <string>
#include <vector>

enum class TextureRequestPlanBuilder::SceneCollectionResult
{
	Succeeded,
	RecoverableFailure,
	FatalFailure,
};

TextureRequestPlanBuilder::SceneCollectionResult TextureRequestPlanBuilder::CollectSceneRequests(
    const AssetCookerSceneEntry& sceneEntry,
    AssetCookerDiagnostics& diagnostics,
    TextureCookRequestSet& requestSet)
{
	std::vector<TextureCookRequest> sceneRequests;
	std::string errorMessage;
	SourceImportResult importResult;
	if (!ImportedSceneCooker::Import(sceneEntry, AssetCookerCategory_Textures, diagnostics, importResult))
	{
		return SceneCollectionResult::RecoverableFailure;
	}

	if (!MaterialCooker::CollectTextureCookRequests(importResult, sceneRequests, errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, sceneEntry.sourcePath);
		return SceneCollectionResult::RecoverableFailure;
	}

	for (const TextureCookRequest& request : sceneRequests)
	{
		if (!requestSet.Add(request, errorMessage))
		{
			diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, sceneEntry.sourcePath);
			return SceneCollectionResult::FatalFailure;
		}
	}

	return SceneCollectionResult::Succeeded;
}

bool TextureRequestPlanBuilder::Build(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    const std::filesystem::path& outputPath)
{
	TextureCookRequestSet requestSet;
	std::size_t failedSceneCount = 0;
	for (std::size_t sceneIndex = 0; sceneIndex < plan.sceneEntries.size(); ++sceneIndex)
	{
		const AssetCookerSceneEntry& sceneEntry = plan.sceneEntries[sceneIndex];
		const SceneCollectionResult collectionResult = CollectSceneRequests(sceneEntry, diagnostics, requestSet);
		if (collectionResult == SceneCollectionResult::FatalFailure)
		{
			return false;
		}

		failedSceneCount += collectionResult == SceneCollectionResult::RecoverableFailure ? 1u : 0u;
	}

	if (failedSceneCount != 0)
	{
		diagnostics.AddError(
		    AssetCookerCategory_Textures,
		    "Texture request collection failed for " + std::to_string(failedSceneCount) + " scene(s).");
		return false;
	}

	std::string errorMessage;
	if (!DefaultTextureCookRequestBuilder::AppendTo(requestSet, errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Textures, errorMessage);
		return false;
	}

	std::vector<TextureCookRequest> requests;
	requestSet.MoveRequestsTo(requests);
	if (!WriteTextureCookRequestList(outputPath, requests, errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, outputPath);
		return false;
	}

	return true;
}
