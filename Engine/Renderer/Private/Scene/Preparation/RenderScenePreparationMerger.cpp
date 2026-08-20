#include "PCH.h"

#include "Scene/Preparation/RenderScenePreparationMerger.h"

#include "Scene/Preparation/RenderLightPreparation.h"
#include "Scene/Preparation/RenderScenePreparationRun.h"
#include "Tasks/Public/TaskExecutionContext.h"

#include <utility>

TaskResult RenderScenePreparationMerger::Merge(TaskExecutionContext& context)
{
	RenderScenePreparationRun& run = *context.TryGet<RenderScenePreparationRun>();

	RenderLightPreparation::Commit(run.PreparedLights, run.PreparedScene);
	PublishPrimitives(run);
	return TaskResult::Success();
}

void RenderScenePreparationMerger::PublishSceneOutputs(RenderScenePreparationRun& run)
{
	run.PreparedScene.jointMatrices = std::move(run.Deformation.JointMatrices);
	run.PreparedScene.previousJointMatrices = std::move(run.Deformation.PreviousJointMatrices);
	run.PreparedScene.morphWeights = std::move(run.Deformation.MorphWeights);
	run.PreparedScene.previousMorphWeights = std::move(run.Deformation.PreviousMorphWeights);
}

void RenderScenePreparationMerger::PublishPrimitives(RenderScenePreparationRun& run)
{
	run.PreparedScene.primitives = std::move(run.PreparedPrimitives);
}

TaskResult RenderScenePreparationMerger::BuildRayTracingPlan(TaskExecutionContext& context)
{
	RenderScenePreparationRun& run = *context.TryGet<RenderScenePreparationRun>();
	RenderRayTracingWorkPlan& plan = run.PreparedScene.rayTracingWork;

	plan.BlasInputs.reserve(run.PreparedScene.primitives.size());
	plan.ClassicTlasBlasInputIndices.reserve(run.PreparedScene.primitives.size());
	plan.PartitionedTlasBlasInputIndices.reserve(run.PreparedScene.primitives.size());

	for (std::uint32_t primitiveIndex = 0u; primitiveIndex < run.PreparedScene.primitives.size(); ++primitiveIndex)
	{
		const MeshDraw& draw = run.PreparedScene.primitives[primitiveIndex].Draw;
		if (!draw.Geometry.Mesh)
		{
			continue;
		}

		const std::uint32_t blasInputIndex = static_cast<std::uint32_t>(plan.BlasInputs.size());
		plan.BlasInputs.push_back(RenderRayTracingBlasInput{.PrimitiveIndex = primitiveIndex, .GpuSceneSlot = draw.Source.GpuSceneSlot});
		plan.ClassicTlasBlasInputIndices.push_back(blasInputIndex);
		plan.PartitionedTlasBlasInputIndices.push_back(blasInputIndex);
	}

	return TaskResult::Success();
}
