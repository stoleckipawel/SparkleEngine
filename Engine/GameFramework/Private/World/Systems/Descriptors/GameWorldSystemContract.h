#pragma once

#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/ECS/Query.h"
#include "World/Systems/GameSystemGraph.h"

namespace ECS
{
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

}
