#include "PCH.h"

#include "CookedSkeletonAssetWriter.h"

#include "CookedSceneBuild.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <fstream>

class CookedSkeletonAssetStager final
{
  public:
	static bool StageSkeletonAsset(
	    const CookedSkeletonAssetBuild& skeletonAsset,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};

bool CookedSkeletonAssetWriter::StageSkeletonAssets(
    const std::vector<CookedSkeletonAssetBuild>& skeletonAssets,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	for (const CookedSkeletonAssetBuild& skeletonAsset : skeletonAssets)
	{
		if (!CookedSkeletonAssetStager::StageSkeletonAsset(
		        skeletonAsset,
		        outPublication,
		        outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

bool CookedSkeletonAssetStager::StageSkeletonAsset(
    const CookedSkeletonAssetBuild& skeletonAsset,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	const std::filesystem::path outputPath =
	    Paths::CookedSkeletonAsset(skeletonAsset.assetId);
	const std::filesystem::path stagedOutputPath =
	    Files::BuildTemporaryPath(outputPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedOutputPath);
	outPublication.push_back({stagedOutputPath, outputPath});

	const Assets::CookedSkeletonAssetHeader header{
	    .fileHeader = {Assets::kCookedSkeletonAssetMagic, Assets::kCookedSkeletonAssetVersion},
	    .jointCount = static_cast<std::uint32_t>(skeletonAsset.joints.size()),
	    .jointStride = sizeof(Assets::CookedSkeletonJointRecord),
	    .flags = 0};

	std::ofstream output;
	if (!Files::TryOpenBinaryOutput(stagedOutputPath, output, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, skeletonAsset.joints, outErrorMessage))
	{
		return false;
	}

	return Files::TryCloseOutput(
	    output,
	    stagedOutputPath,
	    outErrorMessage);
}
