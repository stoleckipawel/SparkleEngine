#pragma once

#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/ECS/Query.h"
#include "World/Systems/GameSystemGraph.h"

namespace ECS
{
	namespace GameWorldSystemIds
	{
		constexpr GameSystemId CameraMovement = MakeGameSystemId("Sparkle.Game.CameraMovement");
		constexpr GameSystemId PlaybackAdvance = MakeGameSystemId("Sparkle.Game.AnimationPlaybackAdvance");
		constexpr GameSystemId PoseEvaluation = MakeGameSystemId("Sparkle.Game.AnimationPoseEvaluation");
		constexpr GameSystemId MorphWeightEvaluation = MakeGameSystemId("Sparkle.Game.MorphWeightEvaluation");
		constexpr GameSystemId SkinningMatrixEvaluation = MakeGameSystemId("Sparkle.Game.SkinningMatrixEvaluation");
		constexpr GameSystemId MorphOutputCommit = MakeGameSystemId("Sparkle.Game.MorphOutputCommit");
		constexpr GameSystemId SystemOutputCommit = MakeGameSystemId("Sparkle.Game.SystemOutputCommit");
		constexpr GameSystemId TransformEvaluation = MakeGameSystemId("Sparkle.Game.TransformEvaluation");
		constexpr GameSystemId CameraDerivedState = MakeGameSystemId("Sparkle.Game.CameraDerivedState");
		constexpr GameSystemId MeshExtraction = MakeGameSystemId("Sparkle.Game.MeshExtraction");
		constexpr GameSystemId ExtractionCommit = MakeGameSystemId("Sparkle.Game.ExtractionCommit");
	}

	using CameraMovementQuery = Query<Write<Camera>, Write<LocalTransform>>;
	using PlaybackAdvanceQuery = Query<Write<AnimationState>>;
	using PoseEvaluationQuery = Query<Read<AnimationState>>;
	using MorphEvaluationQuery = Query<Read<AnimationState>>;
	using TransformEvaluationQuery = Query<Read<LocalTransform>, Write<WorldTransform>>;
	using CameraDerivedStateQuery = Query<Read<LocalTransform>, Read<Camera>, Write<CameraDerivedState>>;
	using MeshExtractionQuery = Query<Read<MeshInstance>, Read<Visibility>, Read<WorldTransform>>;

	namespace GameWorldSystemGrain
	{
		constexpr ParallelForPolicy Camera{.GrainSize = 32, .SerialThreshold = 64, .MaximumPartitions = 4};
		constexpr ParallelForPolicy Animation{.GrainSize = 8, .SerialThreshold = 16, .MaximumPartitions = 16};
		constexpr ParallelForPolicy Pose{.GrainSize = 2, .SerialThreshold = 4, .MaximumPartitions = 16};
		constexpr ParallelForPolicy Transform{.GrainSize = 64, .SerialThreshold = 128, .MaximumPartitions = 16};
		constexpr ParallelForPolicy Extraction{.GrainSize = 64, .SerialThreshold = 128, .MaximumPartitions = 16};
		constexpr ParallelForPolicy SingleItem{.GrainSize = 1, .SerialThreshold = 1, .MaximumPartitions = 1};
	}

	constexpr GameSystemExecutionPolicy ParallelRanges(ParallelForPolicy policy) noexcept
	{
		return {GameSystemExecutionMode::ParallelRanges, policy};
	}
}
