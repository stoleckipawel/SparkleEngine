#include "PCH.h"

#include "Scene/Animations/SceneAnimationDiagnostics.h"

static const auto g_sceneAnimationsLogger = Logging::GetOrCreateLogger("GameFramework.SceneAnimations");

namespace
{
	float ResolveFirstMorphWeight(std::span<const SceneMorphWeightSnapshot> morphWeights) noexcept
	{
		return !morphWeights.empty() && !morphWeights.front().weights.empty() ? morphWeights.front().weights.front() : 0.0f;
	}
}

namespace SceneAnimationDiagnostics
{
	std::uint32_t CountUnsupportedRuntimeChannels(const SceneAnimationClipDesc& clip) noexcept
	{
		std::uint32_t unsupportedChannelCount = 0;
		for (const SceneAnimationChannel& channel : clip.channels)
		{
			if (channel.targetPath == Assets::CookedAnimationTargetPath::Unknown)
			{
				++unsupportedChannelCount;
			}
		}
		return unsupportedChannelCount;
	}

	void LogLoadedClip(const SceneAnimationClipDesc& clip)
	{
		SPDLOG_LOGGER_INFO(
		    g_sceneAnimationsLogger,
		    "SceneAnimations: loaded clip '{}' duration={:.3f}s channels={} keyframes={} animationAsset=0x{:016X} targetSkeleton=0x{:016X}",
		    clip.name,
		    clip.durationSeconds,
		    clip.channelCount,
		    clip.keyframeCount,
		    clip.animationAssetId,
		    clip.targetSkeletonAssetId);
	}

	void LogUnsupportedRuntimeChannels(const SceneAnimationClipDesc& clip, std::uint32_t unsupportedRuntimeChannelCount)
	{
		SPDLOG_LOGGER_WARN(
		    g_sceneAnimationsLogger,
		    "SceneAnimations: clip '{}' has {} unsupported runtime animation channel(s); playback applies translation, rotation, scale, and skeletal morph-weight channels.",
		    clip.name,
		    unsupportedRuntimeChannelCount);
	}

	void LogPlayback(
	    std::span<const SceneAnimationClipDesc> clips,
	    std::span<const SceneAnimationPoseSnapshot> poses,
	    std::span<const SceneMorphWeightSnapshot> morphWeights)
	{
		const SceneAnimationPoseSnapshot* firstPose = poses.empty() ? nullptr : &poses.front();
		SPDLOG_LOGGER_INFO(
		    g_sceneAnimationsLogger,
		    "SceneAnimations: playback active clips={} poses={} morphWeights={} firstMorphWeight={:.3f} firstClip='{}' time={:.3f}s joints={}",
		    clips.size(),
		    poses.size(),
		    morphWeights.size(),
		    ResolveFirstMorphWeight(morphWeights),
		    firstPose != nullptr ? firstPose->clipName.c_str() : "<weights-only>",
		    firstPose != nullptr ? firstPose->playbackTimeSeconds : 0.0f,
		    firstPose != nullptr ? firstPose->jointCount : 0u);
	}
}
