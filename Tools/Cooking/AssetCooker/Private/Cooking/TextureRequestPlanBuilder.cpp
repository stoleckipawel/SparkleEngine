#include "TextureRequestPlanBuilder.h"

#include "Core/Public/Diagnostics/Error.h"
#include "DefaultTextureCookRequestBuilder.h"
#include "ImportedSceneCooker.h"
#include "MaterialCooker.h"
#include "SourceSceneImporter.h"
#include "TextureCookRequestList.h"

#include <string>
#include <vector>

void TextureRequestPlanBuilder::CollectSceneRequests(const AssetCookerSceneEntry& sceneEntry, TextureCookRequestSet& requestSet)
{
	const SourceImportOutput importOutput = ImportedSceneCooker::Import(sceneEntry);
	const std::vector<TextureCookRequest> sceneRequests = MaterialCooker::CollectTextureCookRequests(importOutput);

	for (const TextureCookRequest& request : sceneRequests)
	{
		requestSet.Add(request);
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
		try
		{
			CollectSceneRequests(sceneEntry, requestSet);
		}
		catch (const Diagnostics::Error& error)
		{
			diagnostics.AddError(AssetCookerCategory_Textures, error.what(), sceneEntry.sourcePath);
			++failedSceneCount;
		}
	}

	if (failedSceneCount != 0)
	{
		diagnostics.AddError(
		    AssetCookerCategory_Textures,
		    "Texture request collection failed for " + std::to_string(failedSceneCount) + " scene(s).");
		return false;
	}

	try
	{
		DefaultTextureCookRequestBuilder::AppendTo(requestSet);
	}
	catch (const Diagnostics::Error& error)
	{
		diagnostics.AddError(AssetCookerCategory_Textures, error.what());
		return false;
	}

	std::vector<TextureCookRequest> requests = requestSet.ReleaseRequests();
	try
	{
		WriteTextureCookRequestList(outputPath, requests);
	}
	catch (const Diagnostics::Error& error)
	{
		diagnostics.AddError(AssetCookerCategory_Textures, error.what(), outputPath);
		return false;
	}

	return true;
}
