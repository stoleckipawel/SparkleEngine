#include "PCH.h"

#include "CookedSkeletonAssetWriter.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <fstream>

bool CookedSkeletonAssetWriter::StageSkeletonAssets(
    const std::vector<CookedSkeletonAssetBuild>& skeletonAssets,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	for (const CookedSkeletonAssetBuild& skeletonAsset : skeletonAssets)
	{
		const std::filesystem::path outputPath = Paths::CookedSkeletonAsset(skeletonAsset.assetId);
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
		if (!Files::TryOpenBinaryOutput(stagedOutputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, skeletonAsset.joints, outErrorMessage))
		{
			return false;
		}

		if (!Files::TryCloseOutput(output, stagedOutputPath, outErrorMessage))
		{
			return false;
		}

	}

	outErrorMessage.clear();
	return true;
}
