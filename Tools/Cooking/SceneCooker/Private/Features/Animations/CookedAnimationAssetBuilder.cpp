#include "PCH.h"

#include "Features/Animations/CookedAnimationAssetBuilder.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Hash/HashUtils.h"

#include <format>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

class CookedAnimationTranslation final
{
public:
	static Assets::CookedAssetId BuildAnimationAssetId(std::string_view sceneAssetId, std::uint32_t sourceAnimationIndex)
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#animation#" + std::to_string(sourceAnimationIndex));
	}

	static Assets::CookedAnimationInterpolation ToCookedInterpolation(ImportedAnimationInterpolation interpolation)
	{
		switch (interpolation)
		{
			case ImportedAnimationInterpolation::Linear:
				return Assets::CookedAnimationInterpolation::Linear;
			case ImportedAnimationInterpolation::Step:
				return Assets::CookedAnimationInterpolation::Step;
			case ImportedAnimationInterpolation::CubicSpline:
				return Assets::CookedAnimationInterpolation::CubicSpline;
		}
		throw Diagnostics::Error("Imported animation uses an unsupported interpolation mode.");
	}

	static Assets::CookedAnimationTargetPath ToCookedTargetPath(ImportedAnimationTargetPath targetPath)
	{
		switch (targetPath)
		{
			case ImportedAnimationTargetPath::Translation:
				return Assets::CookedAnimationTargetPath::Translation;
			case ImportedAnimationTargetPath::Rotation:
				return Assets::CookedAnimationTargetPath::Rotation;
			case ImportedAnimationTargetPath::Scale:
				return Assets::CookedAnimationTargetPath::Scale;
			case ImportedAnimationTargetPath::Weights:
				return Assets::CookedAnimationTargetPath::Weights;
			case ImportedAnimationTargetPath::Unknown:
				break;
		}
		throw Diagnostics::Error("Imported animation uses an unsupported target path.");
	}

	static Assets::CookedAssetId ResolveTargetSkeletonAssetId(const CookedSceneBuild& build, const ImportedAnimationClip& clip) noexcept
	{
		return build.manifest.skeletonRefs[clip.targetSkeletonIndex].skeletonAssetId;
	}

	static CookedAnimationAssetBuild BuildAsset(
	    const SourceImportOutput& importOutput,
	    std::string_view sceneAssetId,
	    const ImportedAnimationClip& importedClip,
	    std::size_t clipIndex,
	    const CookedSceneBuild& build);
	static void AppendChannel(
	    const ImportedAnimationClip& importedClip,
	    const ImportedAnimationChannel& importedChannel,
	    std::size_t clipIndex,
	    std::size_t channelIndex,
	    const CookedSkeletonAssetBuild& skeleton,
	    CookedAnimationAssetBuild& asset);
};

CookedAnimationAssetBuild CookedAnimationTranslation::BuildAsset(
    const SourceImportOutput& importOutput,
    std::string_view sceneAssetId,
    const ImportedAnimationClip& importedClip,
    std::size_t clipIndex,
    const CookedSceneBuild& build)
{
	if (!importedClip.IsValid() || importedClip.name.size() >= sizeof(Assets::CookedAnimationAssetHeader::name)
	    || importedClip.targetSkeletonIndex >= build.outputs.skeletonAssets.size()
	    || importedClip.targetSkeletonIndex >= build.manifest.skeletonRefs.size())
	{
		throw Diagnostics::Error(std::format("Imported animation clip {} is invalid.", clipIndex));
	}

	CookedAnimationAssetBuild asset;
	asset.assetId = BuildAnimationAssetId(sceneAssetId, importedClip.sourceAnimationIndex);
	asset.targetSkeletonAssetId = ResolveTargetSkeletonAssetId(build, importedClip);
	if (asset.targetSkeletonAssetId == Assets::InvalidCookedAssetId)
	{
		throw Diagnostics::Error(std::format("Imported animation clip {} has no target skeleton.", clipIndex));
	}
	asset.sourceAnimationIndex = importedClip.sourceAnimationIndex;
	asset.durationSeconds = importedClip.durationSeconds;
	asset.name = importedClip.name;
	asset.sourcePath = importOutput.GetSourcePath();
	asset.channels.reserve(importedClip.channels.size());

	const CookedSkeletonAssetBuild& skeleton = build.outputs.skeletonAssets[importedClip.targetSkeletonIndex];
	for (std::size_t channelIndex = 0; channelIndex < importedClip.channels.size(); ++channelIndex)
	{
		AppendChannel(importedClip, importedClip.channels[channelIndex], clipIndex, channelIndex, skeleton, asset);
	}
	return asset;
}

void CookedAnimationTranslation::AppendChannel(
    const ImportedAnimationClip& importedClip,
    const ImportedAnimationChannel& importedChannel,
    std::size_t clipIndex,
    std::size_t channelIndex,
    const CookedSkeletonAssetBuild& skeleton,
    CookedAnimationAssetBuild& asset)
{
	if (importedChannel.samplerIndex >= importedClip.samplers.size())
	{
		throw Diagnostics::Error(
		    std::format("Imported animation clip {} channel {} references an invalid sampler.", clipIndex, channelIndex));
	}

	const ImportedAnimationSampler& sampler = importedClip.samplers[importedChannel.samplerIndex];
	if (!sampler.IsValid())
	{
		throw Diagnostics::Error(
		    std::format("Imported animation clip {} channel {} has incomplete sampler data.", clipIndex, channelIndex));
	}
	const Assets::CookedAnimationTargetPath targetPath = ToCookedTargetPath(importedChannel.targetPath);
	const Assets::CookedAnimationInterpolation interpolation = ToCookedInterpolation(sampler.interpolation);
	if (importedChannel.targetPath != ImportedAnimationTargetPath::Weights && importedChannel.targetJointIndex >= skeleton.joints.size())
	{
		throw Diagnostics::Error(
		    std::format("Imported animation clip {} channel {} references an invalid target joint.", clipIndex, channelIndex));
	}
	if (asset.keyframes.size() > (std::numeric_limits<std::uint32_t>::max)()
	    || sampler.keyframes.size() > (std::numeric_limits<std::uint32_t>::max)() - asset.keyframes.size())
	{
		throw Diagnostics::Error(std::format("Imported animation clip {} channel {} has too many keyframes.", clipIndex, channelIndex));
	}

	float previousTime = -1.0f;
	for (const ImportedAnimationKeyframe& keyframe : sampler.keyframes)
	{
		if (keyframe.timeSeconds < 0.0f || keyframe.timeSeconds <= previousTime || keyframe.timeSeconds > importedClip.durationSeconds)
		{
			throw Diagnostics::Error(
			    std::format("Imported animation clip {} channel {} has invalid keyframe timing.", clipIndex, channelIndex));
		}
		previousTime = keyframe.timeSeconds;
	}

	const auto firstKeyframe = static_cast<std::uint32_t>(asset.keyframes.size());
	for (const ImportedAnimationKeyframe& importedKeyframe : sampler.keyframes)
	{
		asset.keyframes.push_back(
		    {.timeSeconds = importedKeyframe.timeSeconds,
		        .value = importedKeyframe.value,
		        .inTangent = importedKeyframe.inTangent,
		        .outTangent = importedKeyframe.outTangent});
	}
	asset.channels.push_back(
	    {.targetPath = targetPath,
	        .interpolation = interpolation,
	        .targetNodeIndex = importedChannel.targetNodeIndex,
	        .targetJointIndex = importedChannel.targetJointIndex,
	        .firstKeyframe = firstKeyframe,
	        .keyframeCount = static_cast<std::uint32_t>(sampler.keyframes.size())});
}

void CookedAnimationAssetBuilder::Build(const SourceImportOutput& importOutput, std::string_view sceneAssetId, CookedSceneBuild& outBuild)
{
	outBuild.outputs.animationAssets.clear();
	outBuild.manifest.animationReferences.clear();
	outBuild.outputs.animationAssets.reserve(importOutput.scene.animations.size());
	outBuild.manifest.animationReferences.reserve(importOutput.scene.animations.size());

	std::unordered_set<std::uint32_t> sourceAnimationIndices;
	for (std::size_t clipIndex = 0; clipIndex < importOutput.scene.animations.size(); ++clipIndex)
	{
		const ImportedAnimationClip& importedClip = importOutput.scene.animations[clipIndex];
		if (!sourceAnimationIndices.insert(importedClip.sourceAnimationIndex).second)
		{
			throw Diagnostics::Error(std::format("Imported animation clip {} duplicates a source animation index.", clipIndex));
		}

		CookedAnimationAssetBuild animationAsset =
		    CookedAnimationTranslation::BuildAsset(importOutput, sceneAssetId, importedClip, clipIndex, outBuild);

		outBuild.manifest.animationReferences.push_back(
		    {.animationAssetId = animationAsset.assetId, .sourceAnimationIndex = importedClip.sourceAnimationIndex, .flags = 0});
		outBuild.outputs.animationAssets.push_back(std::move(animationAsset));
	}
}
