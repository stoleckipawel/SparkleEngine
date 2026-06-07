#include "PCH.h"

#include "CookedSkeletonAssetWriter.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <fstream>

bool CookedSkeletonAssetWriter::WriteSkeletonAssets(
    const std::vector<CookedSkeletonAssetBuild>& skeletonAssets,
    std::string& outErrorMessage)
{
	for (const CookedSkeletonAssetBuild& skeletonAsset : skeletonAssets)
	{
		const std::filesystem::path outputPath = Paths::CookedSkeletonAsset(skeletonAsset.assetId);
		const Assets::CookedSkeletonAssetHeader header{
		    .fileHeader = {Assets::kCookedSkeletonAssetMagic, Assets::kCookedSkeletonAssetVersion},
		    .jointCount = static_cast<std::uint32_t>(skeletonAsset.joints.size()),
		    .jointStride = sizeof(Assets::CookedSkeletonJointRecord),
		    .flags = 0};

		std::ofstream output;
		if (!Files::TryOpenBinaryOutput(outputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, skeletonAsset.joints, outErrorMessage))
		{
			return false;
		}

		if (!Files::TryCloseOutput(output, outputPath, outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}
