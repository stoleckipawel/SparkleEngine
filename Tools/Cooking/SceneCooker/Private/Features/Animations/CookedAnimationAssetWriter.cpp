#include "PCH.h"

#include "CookedAnimationAssetWriter.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace
{
	void CopyName(std::string_view sourceName, char (&destination)[64]) noexcept
	{
		std::memset(destination, 0, sizeof(destination));
		const std::size_t copyLength = (std::min)(sourceName.size(), sizeof(destination) - 1u);
		std::memcpy(destination, sourceName.data(), copyLength);
	}
}

bool CookedAnimationAssetWriter::WriteAnimationAssets(
    const std::vector<CookedAnimationAssetBuild>& animationAssets,
    std::string& outErrorMessage)
{
	for (const CookedAnimationAssetBuild& animationAsset : animationAssets)
	{
		const std::filesystem::path outputPath = Paths::CookedAnimationAsset(animationAsset.assetId);
		Assets::CookedAnimationAssetHeader header{
		    .fileHeader = {Assets::kCookedAnimationAssetMagic, Assets::kCookedAnimationAssetVersion},
		    .targetSkeletonAssetId = animationAsset.targetSkeletonAssetId,
		    .sourceAnimationIndex = animationAsset.sourceAnimationIndex,
		    .durationSeconds = animationAsset.durationSeconds,
		    .channelCount = static_cast<std::uint32_t>(animationAsset.channels.size()),
		    .keyframeCount = static_cast<std::uint32_t>(animationAsset.keyframes.size()),
		    .channelStride = sizeof(Assets::CookedAnimationChannelRecord),
		    .keyframeStride = sizeof(Assets::CookedAnimationKeyframeRecord),
		    .flags = 0};
		CopyName(animationAsset.name, header.name);

		std::ofstream output;
		if (!Files::TryOpenBinaryOutput(outputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, animationAsset.channels, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, animationAsset.keyframes, outErrorMessage))
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
