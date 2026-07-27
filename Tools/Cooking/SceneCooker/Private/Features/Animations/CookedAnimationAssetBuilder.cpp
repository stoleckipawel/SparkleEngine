#include "PCH.h"

#include "Features/Animations/CookedAnimationAssetBuilder.h"

#include "Core/Public/Hash/HashUtils.h"

#include <string>
#include <utility>

class CookedAnimationTranslation final
{
  public:
	static Assets::CookedAssetId BuildAnimationAssetId(std::string_view sceneAssetId, std::uint32_t sourceAnimationIndex)
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#animation#" + std::to_string(sourceAnimationIndex));
	}

	static Assets::CookedAnimationInterpolation ToCookedInterpolation(ImportedAnimationInterpolation interpolation) noexcept
	{
		switch (interpolation)
		{
			case ImportedAnimationInterpolation::Step: return Assets::CookedAnimationInterpolation::Step;
			case ImportedAnimationInterpolation::CubicSpline: return Assets::CookedAnimationInterpolation::CubicSpline;
			case ImportedAnimationInterpolation::Linear:
			default: return Assets::CookedAnimationInterpolation::Linear;
		}
	}

	static Assets::CookedAnimationTargetPath ToCookedTargetPath(ImportedAnimationTargetPath targetPath) noexcept
	{
		switch (targetPath)
		{
			case ImportedAnimationTargetPath::Translation: return Assets::CookedAnimationTargetPath::Translation;
			case ImportedAnimationTargetPath::Rotation: return Assets::CookedAnimationTargetPath::Rotation;
			case ImportedAnimationTargetPath::Scale: return Assets::CookedAnimationTargetPath::Scale;
			case ImportedAnimationTargetPath::Weights: return Assets::CookedAnimationTargetPath::Weights;
			case ImportedAnimationTargetPath::Unknown:
			default: return Assets::CookedAnimationTargetPath::Unknown;
		}
	}

	static Assets::CookedAssetId ResolveTargetSkeletonAssetId(const CookedSceneBuild& build, const ImportedAnimationClip& clip) noexcept
	{
		return clip.targetSkeletonIndex < build.manifest.skeletonRefs.size()
		           ? build.manifest.skeletonRefs[clip.targetSkeletonIndex].skeletonAssetId
		           : Assets::InvalidCookedAssetId;
	}
};

void CookedAnimationAssetBuilder::Build(
	const SourceImportResult& importResult, std::string_view sceneAssetId, CookedSceneBuild& outBuild)
{
	outBuild.outputs.animationAssets.clear();
	outBuild.manifest.animationReferences.clear();
	outBuild.outputs.animationAssets.reserve(importResult.scene.animations.size());
	outBuild.manifest.animationReferences.reserve(importResult.scene.animations.size());

	for (const ImportedAnimationClip& importedClip : importResult.scene.animations)
	{
		if (!importedClip.IsValid()) continue;
		CookedAnimationAssetBuild animationAsset;
		animationAsset.assetId = CookedAnimationTranslation::BuildAnimationAssetId(sceneAssetId, importedClip.sourceAnimationIndex);
		animationAsset.targetSkeletonAssetId = CookedAnimationTranslation::ResolveTargetSkeletonAssetId(outBuild, importedClip);
		animationAsset.sourceAnimationIndex = importedClip.sourceAnimationIndex;
		animationAsset.durationSeconds = importedClip.durationSeconds;
		animationAsset.name = importedClip.name;
		animationAsset.sourcePath = importResult.scene.sourcePath;
		animationAsset.channels.reserve(importedClip.channels.size());

		for (const ImportedAnimationChannel& importedChannel : importedClip.channels)
		{
			if (importedChannel.samplerIndex >= importedClip.samplers.size()) continue;
			const ImportedAnimationSampler& sampler = importedClip.samplers[importedChannel.samplerIndex];
			if (!sampler.IsValid()) continue;
			const auto firstKeyframe = static_cast<std::uint32_t>(animationAsset.keyframes.size());
			for (const ImportedAnimationKeyframe& importedKeyframe : sampler.keyframes)
				animationAsset.keyframes.push_back({.timeSeconds = importedKeyframe.timeSeconds,
				                                    .value = importedKeyframe.value,
				                                    .inTangent = importedKeyframe.inTangent,
				                                    .outTangent = importedKeyframe.outTangent});
			animationAsset.channels.push_back({.targetPath = CookedAnimationTranslation::ToCookedTargetPath(importedChannel.targetPath),
			                                   .interpolation = CookedAnimationTranslation::ToCookedInterpolation(sampler.interpolation),
			                                   .targetNodeIndex = importedChannel.targetNodeIndex,
			                                   .targetJointIndex = importedChannel.targetJointIndex,
			                                   .firstKeyframe = firstKeyframe,
			                                   .keyframeCount = static_cast<std::uint32_t>(sampler.keyframes.size())});
		}

		if (animationAsset.channels.empty() || animationAsset.keyframes.empty()) continue;
		outBuild.manifest.animationReferences.push_back({.animationAssetId = animationAsset.assetId,
		                                                 .sourceAnimationIndex = importedClip.sourceAnimationIndex,
		                                                 .flags = 0});
		outBuild.outputs.animationAssets.push_back(std::move(animationAsset));
	}
}
