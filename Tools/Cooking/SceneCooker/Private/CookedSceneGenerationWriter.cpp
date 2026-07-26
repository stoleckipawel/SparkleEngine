#include "PCH.h"

#include "CookedSceneGenerationWriter.h"

#include "CookedAnimationAssetWriter.h"
#include "CookedMeshAssetWriter.h"
#include "CookedSkeletonAssetWriter.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"

bool CookedSceneGenerationWriter::Publish(
    std::span<const CookedSceneBuild* const> builds,
    std::string& outErrorMessage)
{
	std::vector<Files::FilePublication> publication;
	if (!StageAssets(builds, publication, outErrorMessage) ||
	    !SceneCooker::StageManifestsAndRegistry(
	        builds,
	        publication,
	        outErrorMessage))
	{
		Cleanup(publication);
		return false;
	}

	if (!Files::TryPublishFileSet(publication, outErrorMessage))
	{
		Cleanup(publication);
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool CookedSceneGenerationWriter::StageAssets(
    std::span<const CookedSceneBuild* const> builds,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	for (const CookedSceneBuild* build : builds)
	{
		if (build == nullptr)
		{
			outErrorMessage = "Scene generation contains a null build.";
			return false;
		}

		if (!CookedMeshAssetWriter::StageMeshAssets(
		        build->outputs.meshAssets,
		        outPublication,
		        outErrorMessage) ||
		    !MaterialCooker::StageMaterialAssets(
		        build->outputs.materialAssets,
		        outPublication,
		        outErrorMessage) ||
		    !CookedSkeletonAssetWriter::StageSkeletonAssets(
		        build->outputs.skeletonAssets,
		        outPublication,
		        outErrorMessage) ||
		    !CookedAnimationAssetWriter::StageAnimationAssets(
		        build->outputs.animationAssets,
		        outPublication,
		        outErrorMessage))
		{
			return false;
		}
	}

	return true;
}

void CookedSceneGenerationWriter::Cleanup(
    std::span<const Files::FilePublication> publication) noexcept
{
	for (const Files::FilePublication& file : publication)
	{
		Files::CleanupTemporaryFile(file.StagedPath);
	}
}
