#include "PCH.h"

#include "CookedSceneGenerationWriter.h"

#include "CookedSceneBuild.h"
#include "CookedAnimationAssetWriter.h"
#include "CookedMeshAssetWriter.h"
#include "CookedSkeletonAssetWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"

#include <vector>

class CookedSceneGenerationWriterOperations final
{
  public:
	static bool StageAssets(
	    std::span<const CookedSceneBuild* const> builds,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static void Cleanup(
	    std::span<const Files::FilePublication> publication) noexcept;
};

bool CookedSceneGenerationWriter::Publish(
    std::span<const CookedSceneBuild* const> builds,
    std::string& outErrorMessage)
{
	std::vector<Files::FilePublication> publication;
	if (!CookedSceneGenerationWriterOperations::StageAssets(
	        builds,
	        publication,
	        outErrorMessage) ||
	    !SceneCooker::StageManifestsAndRegistry(
	        builds,
	        publication,
	        outErrorMessage))
	{
		CookedSceneGenerationWriterOperations::Cleanup(publication);
		return false;
	}

	if (!Files::TryPublishFileSet(publication, outErrorMessage))
	{
		CookedSceneGenerationWriterOperations::Cleanup(publication);
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool CookedSceneGenerationWriterOperations::StageAssets(
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

void CookedSceneGenerationWriterOperations::Cleanup(
    std::span<const Files::FilePublication> publication) noexcept
{
	for (const Files::FilePublication& file : publication)
	{
		Files::CleanupTemporaryFile(file.StagedPath);
	}
}
