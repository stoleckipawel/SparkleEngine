#include "PCH.h"

#include "CookedSkeletonAssetWriter.h"

#include "CookedSceneBuild.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <fstream>

class CookedSkeletonAssetStager final
{
public:
	static void StageSkeletonAsset(const CookedSkeletonAssetBuild& skeletonAsset, std::vector<Files::FilePublication>& outPublication);
};

void CookedSkeletonAssetWriter::StageSkeletonAssets(
    const std::vector<CookedSkeletonAssetBuild>& skeletonAssets,
    std::vector<Files::FilePublication>& outPublication)
{
	for (const CookedSkeletonAssetBuild& skeletonAsset : skeletonAssets)
	{
		CookedSkeletonAssetStager::StageSkeletonAsset(skeletonAsset, outPublication);
	}
}

void CookedSkeletonAssetStager::StageSkeletonAsset(
    const CookedSkeletonAssetBuild& skeletonAsset,
    std::vector<Files::FilePublication>& outPublication)
{
	const std::filesystem::path outputPath = Paths::CookedSkeletonAsset(skeletonAsset.assetId);
	const std::filesystem::path stagedOutputPath = Files::BuildTemporaryPath(outputPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedOutputPath);
	outPublication.push_back({stagedOutputPath, outputPath});

	const Assets::CookedSkeletonAssetHeader header{
	    .fileHeader = {Assets::kCookedSkeletonAssetMagic},
	    .jointCount = static_cast<std::uint32_t>(skeletonAsset.joints.size()),
	    .jointStride = sizeof(Assets::CookedSkeletonJointRecord),
	    .flags = 0};

	std::string errorMessage;
	std::ofstream output;
	if (!Files::TryOpenBinaryOutput(stagedOutputPath, output, errorMessage)
	    || !Files::BinaryStreamWriter::WriteValue(output, header, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(output, skeletonAsset.joints, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}

	if (!Files::TryCloseOutput(output, stagedOutputPath, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}
}
