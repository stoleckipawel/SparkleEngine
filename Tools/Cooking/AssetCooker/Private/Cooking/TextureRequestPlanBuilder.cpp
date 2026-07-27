#include "TextureRequestPlanBuilder.h"

#include "DefaultTextureCookRequestBuilder.h"
#include "ImportedSceneCooker.h"
#include "MaterialCooker.h"
#include "SourceSceneImporter.h"
#include "TextureCookRequestList.h"

#include <string>
#include <vector>

class TextureRequestPlanBuilderImplementation final
{
  public:
	enum class SceneRequestCollectionResult
	{
		Succeeded,
		RecoverableFailure,
		FatalFailure,
	};

	static SceneRequestCollectionResult CollectSceneRequests(
	    const AssetCookerSceneEntry& sceneEntry,
	    AssetCookerDiagnostics& diagnostics,
	    TextureCookRequestSet& requestSet)
	{
		std::vector<TextureCookRequest> sceneRequests;
		std::string errorMessage;
		SourceImportResult importResult;
		if (!ImportedSceneCooker::Import(
				    sceneEntry,
				    AssetCookerCategory_Textures,
				    diagnostics,
		    importResult))
		{
			return SceneRequestCollectionResult::RecoverableFailure;
		}

		if (!MaterialCooker::CollectTextureCookRequests(
		        importResult,
		        sceneRequests,
		        errorMessage))
		{
			diagnostics.AddError(
			    AssetCookerCategory_Textures,
			    errorMessage,
			    sceneEntry.sourcePath);
			return SceneRequestCollectionResult::RecoverableFailure;
		}

		for (const TextureCookRequest& request : sceneRequests)
		{
			if (!requestSet.Add(request, errorMessage))
			{
				diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, sceneEntry.sourcePath);
				return SceneRequestCollectionResult::FatalFailure;
			}
		}

		return SceneRequestCollectionResult::Succeeded;
	}
};

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
		const TextureRequestPlanBuilderImplementation::SceneRequestCollectionResult collectionResult = TextureRequestPlanBuilderImplementation::CollectSceneRequests(sceneEntry, diagnostics, requestSet);
		if (collectionResult == TextureRequestPlanBuilderImplementation::SceneRequestCollectionResult::FatalFailure)
		{
			return false;
		}
		failedSceneCount += collectionResult == TextureRequestPlanBuilderImplementation::SceneRequestCollectionResult::RecoverableFailure ? 1u : 0u;
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
