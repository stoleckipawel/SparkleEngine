#include "PCH.h"

#include "Scene/Animations/SceneAnimations.h"

#include "Scene/Animations/SceneAnimationDiagnostics.h"
#include "Scene/Animations/SceneAnimationPoseEvaluator.h"
#include "Scene/Animations/SceneMorphWeightEvaluator.h"
#include "Scene/Skeletons/SceneSkeletons.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	void LogPlaybackDiagnostics(
	    std::span<const SceneAnimationClipDesc> clips,
	    std::span<const SceneAnimationPoseSnapshot> activePoses,
	    std::span<const SceneMorphWeightSnapshot> activeMorphWeights,
	    std::uint32_t& inOutDiagnosticLogCount)
	{
		if (inOutDiagnosticLogCount >= 3u || (activePoses.empty() && activeMorphWeights.empty()))
		{
			return;
		}

		++inOutDiagnosticLogCount;
		SceneAnimationDiagnostics::LogPlayback(clips, activePoses, activeMorphWeights);
	}
}

void SceneAnimations::Clear() noexcept
{
	m_clips.clear();
	m_playbackStates.clear();
	m_activePoses.clear();
	m_activeMorphWeights.clear();
	m_playbackDiagnosticLogCount = 0;
}

void SceneAnimations::AppendClips(std::vector<SceneAnimationClipDesc>&& clips)
{
	m_clips.reserve(m_clips.size() + clips.size());
	m_playbackStates.reserve(m_playbackStates.size() + clips.size());
	for (SceneAnimationClipDesc& clip : clips)
	{
		const std::uint32_t unsupportedRuntimeChannelCount = SceneAnimationDiagnostics::CountUnsupportedRuntimeChannels(clip);
		SceneAnimationDiagnostics::LogLoadedClip(clip);
		if (unsupportedRuntimeChannelCount > 0u)
		{
			SceneAnimationDiagnostics::LogUnsupportedRuntimeChannels(clip, unsupportedRuntimeChannelCount);
		}
		m_clips.push_back(std::move(clip));
		m_playbackStates.push_back(PlaybackState{});
	}
}

void SceneAnimations::Update(float deltaSeconds, const SceneSkeletons& skeletons)
{
	m_activePoses.clear();
	m_activePoses.reserve(m_clips.size());
	m_activeMorphWeights.clear();

	for (std::size_t clipIndex = 0; clipIndex < m_clips.size(); ++clipIndex)
	{
		const SceneAnimationClipDesc& clip = m_clips[clipIndex];
		PlaybackState& playback = m_playbackStates[clipIndex];
		if (!playback.paused && clip.durationSeconds > 0.0f)
		{
			playback.playbackTimeSeconds += (std::max)(0.0f, deltaSeconds) * playback.speed;
			playback.playbackTimeSeconds = playback.looping ? std::fmod(playback.playbackTimeSeconds, clip.durationSeconds)
			                                                : std::min(playback.playbackTimeSeconds, clip.durationSeconds);
		}
		SceneAnimationPoseEvaluator::AppendMatchingPose(clip, playback.playbackTimeSeconds, skeletons, m_activePoses);
		SceneMorphWeightEvaluator::AppendSnapshots(clip, playback.playbackTimeSeconds, m_activeMorphWeights);
	}

	LogPlaybackDiagnostics(m_clips, m_activePoses, m_activeMorphWeights, m_playbackDiagnosticLogCount);
}

SceneAnimationSnapshot SceneAnimations::CaptureSnapshot() const
{
	SceneAnimationSnapshot snapshot;
	snapshot.poses = m_activePoses;
	snapshot.morphWeights = m_activeMorphWeights;
	return snapshot;
}
