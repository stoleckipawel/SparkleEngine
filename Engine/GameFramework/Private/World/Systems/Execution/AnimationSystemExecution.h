#pragma once

#include "World/Systems/Descriptors/GameWorldSystemContract.h"

class GameWorldResourceStores;

namespace ECS
{
	class GameWorldState;

	class AnimationSystemExecution final
	{
	  public:
		AnimationSystemExecution(
		    GameWorldState& state,
		    GameWorldResourceStores& resources,
		    float deltaSeconds,
		    const StructureFrozenEpoch& epoch);

		std::uint32_t GetPlaybackCount() const noexcept;
		std::uint32_t GetPoseCount() const noexcept;
		std::uint32_t GetMorphSampleCount() const noexcept;
		std::uint32_t GetSkinningCount() const noexcept;
		bool RunPlayback(std::uint32_t begin, std::uint32_t end);
		bool RunPose(std::uint32_t begin, std::uint32_t end);
		bool RunMorphSamples(std::uint32_t begin, std::uint32_t end);
		bool RunSkinning(std::uint32_t begin, std::uint32_t end);
		bool CommitMorphOutputs(std::uint32_t begin, std::uint32_t end);

	  private:
		GameWorldState& m_state;
		GameWorldResourceStores& m_resources;
		float m_deltaSeconds = 0.0f;
		PlaybackAdvanceQuery m_playbackQuery;
		PoseEvaluationQuery m_poseQuery;
		MorphEvaluationQuery m_morphQuery;
	};
}
