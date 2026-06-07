#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

class SceneSkeletons;

class SPARKLE_ENGINE_API SceneAnimations final
{
  public:
	void Clear() noexcept;
	void AppendClips(std::vector<SceneAnimationClipDesc>&& clips);
	void Update(float deltaSeconds, const SceneSkeletons& skeletons);
	SceneAnimationSnapshot CaptureSnapshot() const;

	std::size_t GetClipCount() const noexcept { return m_clips.size(); }
	const std::vector<SceneAnimationClipDesc>& GetClips() const noexcept { return m_clips; }
	std::size_t GetActivePoseCount() const noexcept { return m_activePoses.size(); }
	std::span<const SceneMorphWeightSnapshot> GetActiveMorphWeights() const noexcept { return m_activeMorphWeights; }

  private:
	struct PlaybackState final
	{
		float playbackTimeSeconds = 0.0f;
		float speed = 1.0f;
		bool looping = true;
		bool paused = false;
	};

	std::vector<SceneAnimationClipDesc> m_clips;
	std::vector<PlaybackState> m_playbackStates;
	std::vector<SceneAnimationPoseSnapshot> m_activePoses;
	std::vector<SceneMorphWeightSnapshot> m_activeMorphWeights;
	std::uint32_t m_playbackDiagnosticLogCount = 0;
};
