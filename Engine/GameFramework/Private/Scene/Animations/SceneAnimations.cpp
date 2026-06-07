#include "PCH.h"

#include "Scene/Animations/SceneAnimations.h"

#include <utility>

static const auto g_sceneAnimationsLogger = Logging::GetOrCreateLogger("GameFramework.SceneAnimations");

void SceneAnimations::Clear() noexcept
{
	m_clips.clear();
}

void SceneAnimations::AppendClips(std::vector<SceneAnimationClipDesc>&& clips)
{
	m_clips.reserve(m_clips.size() + clips.size());
	for (SceneAnimationClipDesc& clip : clips)
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
		m_clips.push_back(std::move(clip));
	}
}
