#include "PCH.h"

#include "CookedAnimationAssetWriter.h"

#include "CookedSceneBuild.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <algorithm>
#include <cstring>
#include <fstream>

class CookedAnimationAssetStager final
{
  public:
	static bool StageAnimationAsset(
	    const CookedAnimationAssetBuild& animationAsset,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static Assets::CookedAnimationAssetHeader BuildHeader(
	    const CookedAnimationAssetBuild& animationAsset) noexcept;
	static void CopyName(
	    std::string_view sourceName,
	    char (&destination)[64]) noexcept;
};

bool CookedAnimationAssetWriter::StageAnimationAssets(
    const std::vector<CookedAnimationAssetBuild>& animationAssets,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	for (const CookedAnimationAssetBuild& animationAsset : animationAssets)
	{
		if (!CookedAnimationAssetStager::StageAnimationAsset(
		        animationAsset,
		        outPublication,
		        outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

bool CookedAnimationAssetStager::StageAnimationAsset(
    const CookedAnimationAssetBuild& animationAsset,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	const std::filesystem::path outputPath =
	    Paths::CookedAnimationAsset(animationAsset.assetId);
	const std::filesystem::path stagedOutputPath =
	    Files::BuildTemporaryPath(outputPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedOutputPath);
	outPublication.push_back({stagedOutputPath, outputPath});

	const Assets::CookedAnimationAssetHeader header =
	    BuildHeader(animationAsset);
	std::ofstream output;
	if (!Files::TryOpenBinaryOutput(stagedOutputPath, output, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, animationAsset.channels, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, animationAsset.keyframes, outErrorMessage))
	{
		return false;
	}

	return Files::TryCloseOutput(
	    output,
	    stagedOutputPath,
	    outErrorMessage);
}

Assets::CookedAnimationAssetHeader CookedAnimationAssetStager::BuildHeader(
    const CookedAnimationAssetBuild& animationAsset) noexcept
{
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
	return header;
}

void CookedAnimationAssetStager::CopyName(
    std::string_view sourceName,
    char (&destination)[64]) noexcept
{
	std::memset(destination, 0, sizeof(destination));
	const std::size_t copyLength =
	    (std::min)(sourceName.size(), sizeof(destination) - 1u);
	std::memcpy(destination, sourceName.data(), copyLength);
}
