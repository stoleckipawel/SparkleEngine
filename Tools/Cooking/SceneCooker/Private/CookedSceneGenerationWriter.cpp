#include "PCH.h"

#include "CookedSceneGenerationWriter.h"

#include "CookedSceneBuild.h"
#include "CookedAnimationAssetWriter.h"
#include "CookedMeshAssetWriter.h"
#include "CookedSkeletonAssetWriter.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"

#include <vector>

class CookedSceneGenerationStager final
{
public:
	static void StageAssets(std::span<const CookedSceneBuild* const> builds, std::vector<Files::FilePublication>& outPublication);
	static void Cleanup(std::span<const Files::FilePublication> publication) noexcept;
};

void CookedSceneGenerationWriter::Publish(std::span<const CookedSceneBuild* const> builds)
{
	std::vector<Files::FilePublication> publication;
	try
	{
		CookedSceneGenerationStager::StageAssets(builds, publication);
		SceneCooker::StageManifestsAndRegistry(builds, publication);
		std::string errorMessage;
		if (!Files::TryPublishFileSet(publication, errorMessage))
		{
			throw Diagnostics::Error(errorMessage);
		}
	}
	catch (...)
	{
		CookedSceneGenerationStager::Cleanup(publication);
		throw;
	}
}

void CookedSceneGenerationStager::StageAssets(
    std::span<const CookedSceneBuild* const> builds,
    std::vector<Files::FilePublication>& outPublication)
{
	for (const CookedSceneBuild* build : builds)
	{
		if (build == nullptr)
		{
			throw Diagnostics::Error("Scene generation contains a null build.");
		}

		CookedMeshAssetWriter::StageMeshAssets(build->outputs.meshAssets, outPublication);
		MaterialCooker::StageMaterialAssets(build->outputs.materialAssets, outPublication);
		CookedSkeletonAssetWriter::StageSkeletonAssets(build->outputs.skeletonAssets, outPublication);
		CookedAnimationAssetWriter::StageAnimationAssets(build->outputs.animationAssets, outPublication);
	}
}

void CookedSceneGenerationStager::Cleanup(std::span<const Files::FilePublication> publication) noexcept
{
	for (const Files::FilePublication& file : publication)
	{
		Files::CleanupTemporaryFile(file.StagedPath);
	}
}
