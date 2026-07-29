#include "PCH.h"

#include "CookedAnimationAssetWriter.h"

#include "CookedSceneBuild.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <cstring>
#include <fstream>

class CookedAnimationAssetStager final
{
  public:
	static void StageAnimationAsset(
	    const CookedAnimationAssetBuild& animationAsset,
	    std::vector<Files::FilePublication>& outPublication);
	static Assets::CookedAnimationAssetHeader BuildHeader(const CookedAnimationAssetBuild& animationAsset) noexcept;
	static void CopyName(std::string_view sourceName, char (&destination)[64]) noexcept;
};

void CookedAnimationAssetWriter::StageAnimationAssets(
    const std::vector<CookedAnimationAssetBuild>& animationAssets,
    std::vector<Files::FilePublication>& outPublication)
{
	for (const CookedAnimationAssetBuild& animationAsset : animationAssets)
	{
		CookedAnimationAssetStager::StageAnimationAsset(animationAsset, outPublication);
	}
}

void CookedAnimationAssetStager::StageAnimationAsset(
    const CookedAnimationAssetBuild& animationAsset,
    std::vector<Files::FilePublication>& outPublication)
{
	const std::filesystem::path outputPath = Paths::CookedAnimationAsset(animationAsset.assetId);
	const std::filesystem::path stagedOutputPath = Files::BuildTemporaryPath(outputPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedOutputPath);
	outPublication.push_back({stagedOutputPath, outputPath});

	const Assets::CookedAnimationAssetHeader header = BuildHeader(animationAsset);
	std::string errorMessage;
	std::ofstream output;
	if (!Files::TryOpenBinaryOutput(stagedOutputPath, output, errorMessage) ||
	    !Files::BinaryStreamWriter::WriteValue(output, header, errorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, animationAsset.channels, errorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, animationAsset.keyframes, errorMessage))
	{
		throw Diagnostics::Error(std::move(errorMessage));
	}

	if (!Files::TryCloseOutput(output, stagedOutputPath, errorMessage))
	{
		throw Diagnostics::Error(std::move(errorMessage));
	}
}

Assets::CookedAnimationAssetHeader CookedAnimationAssetStager::BuildHeader(const CookedAnimationAssetBuild& animationAsset) noexcept
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

void CookedAnimationAssetStager::CopyName(std::string_view sourceName, char (&destination)[64]) noexcept
{
	std::memset(destination, 0, sizeof(destination));
	std::memcpy(destination, sourceName.data(), sourceName.size());
}
