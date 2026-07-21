#include "TextureRequestPlanBuilder.h"

#include "DefaultTextureCookRequestBuilder.h"
#include "ImportedSceneCooker.h"
#include "MaterialCooker.h"
#include "SourceSceneImporter.h"
#include "TextureCookRequestList.h"
#include "ToolConsole.h"

#include <iostream>
#include <string>
#include <vector>

namespace
{
	enum class SceneRequestCollectionResult
	{
		Succeeded,
		RecoverableFailure,
		FatalFailure,
	};

	SceneRequestCollectionResult CollectSceneRequests(
	    const AssetCookerSceneEntry& sceneEntry,
	    AssetCookerDiagnostics& diagnostics,
	    TextureCookRequestSet& requestSet)
	{
		std::vector<TextureCookRequest> sceneRequests;
		std::string errorMessage;
		const bool collected = ImportedSceneCooker::ImportAndVisit(
		    sceneEntry,
		    AssetCookerCategory_Textures,
		    diagnostics,
		    [&](const SourceImportResult& importResult)
		    {
			    if (!MaterialCooker::CollectTextureCookRequests(importResult, sceneRequests, errorMessage))
			    {
				    diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, sceneEntry.sourcePath);
				    return false;
			    }
			    return true;
		    });
		if (!collected)
		{
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

		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Queued texture references",
		    {ToolConsole::Field("textures", std::to_string(sceneRequests.size())),
		     ToolConsole::QuotedField("scene", sceneEntry.relativePath)});
		return SceneRequestCollectionResult::Succeeded;
	}
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
		ToolConsole::Progress(
		    std::cout,
		    "Collecting",
		    "texture-references",
		    sceneIndex + 1u,
		    plan.sceneEntries.size(),
		    sceneEntry.relativePath,
		    {ToolConsole::Field("origin", sceneEntry.origin)});
		const SceneRequestCollectionResult collectionResult = CollectSceneRequests(sceneEntry, diagnostics, requestSet);
		if (collectionResult == SceneRequestCollectionResult::FatalFailure)
		{
			return false;
		}
		failedSceneCount += collectionResult == SceneRequestCollectionResult::RecoverableFailure ? 1u : 0u;
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

	ToolConsole::Message(
	    std::cout,
	    ToolConsoleSeverity::Info,
	    "Texture request plan",
	    {ToolConsole::Field("textures", std::to_string(requests.size())), ToolConsole::PathField("requestFile", outputPath)});
	return true;
}
