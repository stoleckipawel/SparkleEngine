#include "PCH.h"

#include "SceneData/Preparation/RenderPreparationMerger.h"

#include "SceneData/Preparation/MeshInstanceBatchBuilder.h"
#include "SceneData/Preparation/RenderLightPreparation.h"
#include "SceneData/Preparation/RenderPreparationRun.h"
#include "Tasks/Public/TaskExecutionContext.h"

#include <utility>

TaskResult RenderPreparationMerger::Merge(TaskExecutionContext& context)
{
	RenderPreparationRun& run = *context.TryGet<RenderPreparationRun>();

	PublishDeformation(run);
	RenderLightPreparation::Commit(run.PreparedLights, run.SceneData);

	const std::vector<MeshRenderItem> renderItems = PublishObjects(run);
	PublishBatches(run, renderItems);

	PublishWorkload(run.SceneData);
	return TaskResult::Success();
}

void RenderPreparationMerger::PublishDeformation(RenderPreparationRun& run)
{
	run.SceneData.jointMatrices = run.Deformation.JointMatrices;
	run.SceneData.previousJointMatrices = run.Deformation.PreviousJointMatrices;
	run.SceneData.morphWeights = run.Deformation.MorphWeights;
	run.SceneData.previousMorphWeights = run.Deformation.PreviousMorphWeights;
}

std::vector<MeshRenderItem> RenderPreparationMerger::PublishObjects(RenderPreparationRun& run)
{
	run.SceneData.meshInstances.reserve(run.PreparedObjects.size());
	run.SceneData.meshWorldBounds.reserve(run.PreparedObjects.size());

	std::vector<MeshRenderItem> renderItems;
	renderItems.reserve(run.PreparedObjects.size());

	for (const PreparedRenderObject& object : run.PreparedObjects)
	{
		const std::uint32_t drawIndex = static_cast<std::uint32_t>(run.SceneData.meshInstances.size());
		run.SceneData.meshInstances.push_back(object.Draw);
		run.SceneData.meshWorldBounds.push_back(object.WorldBounds);

		if (!object.RasterVisible)
		{
			continue;
		}

		renderItems.push_back(
		    MeshRenderItem{
		        .Object = object.Object,
		        .DrawIndex = drawIndex,
		        .Material = object.Material,
		        .InstanceGroupIndex = object.InstanceGroupIndex,
		        .Classification = object.MaterialClassification,
		        .CameraDistanceSquared = object.CameraDistanceSquared});
	}

	return renderItems;
}

void RenderPreparationMerger::PublishBatches(
    RenderPreparationRun& run,
    std::span<const MeshRenderItem> renderItems)
{
	MeshInstanceBatchBuildResult batches = MeshInstanceBatchBuilder{}.Build(
	    renderItems,
	    run.SceneData.meshInstances,
	    run.InstanceGroups,
	    MeshInstanceBatchBuildOptions{
	        .EnableAutoBatching = run.EnableAutoBatching,
	        .RequireMaterialBindingSet = true,
	        .CollectDiagnostics = false});

	run.SceneData.rasterMeshInstanceIndices = std::move(batches.RasterInstanceIndices);
	run.SceneData.meshInstanceBatches = std::move(batches.Batches);
}

TaskResult RenderPreparationMerger::BuildRayTracingPlan(TaskExecutionContext& context)
{
	RenderPreparationRun& run = *context.TryGet<RenderPreparationRun>();
	RenderRayTracingWorkPlan& plan = run.SceneData.rayTracingWork;

	plan.BlasInputs.reserve(run.SceneData.meshInstances.size());
	plan.ClassicTlasBlasInputIndices.reserve(run.SceneData.meshInstances.size());
	plan.PartitionedTlasBlasInputIndices.reserve(run.SceneData.meshInstances.size());

	for (std::uint32_t drawIndex = 0u; drawIndex < run.SceneData.meshInstances.size(); ++drawIndex)
	{
		const MeshDraw& draw = run.SceneData.meshInstances[drawIndex];
		if (!draw.Geometry.Mesh)
		{
			continue;
		}

		const std::uint32_t blasInputIndex = static_cast<std::uint32_t>(plan.BlasInputs.size());
		plan.BlasInputs.push_back(
		    RenderRayTracingBlasInput{
		        .MeshInstanceIndex = drawIndex,
		        .GpuSceneSlot = draw.Source.GpuSceneSlot});
		plan.ClassicTlasBlasInputIndices.push_back(blasInputIndex);
		plan.PartitionedTlasBlasInputIndices.push_back(blasInputIndex);
	}

	return TaskResult::Success();
}

void RenderPreparationMerger::PublishWorkload(RenderSceneData& sceneData) noexcept
{
	sceneData.meshWorkload = {};
	sceneData.meshWorkload.jointMatrixCount = static_cast<std::uint32_t>(sceneData.jointMatrices.size());

	for (const std::uint32_t drawIndex : sceneData.rasterMeshInstanceIndices)
	{
		if (drawIndex >= sceneData.meshInstances.size())
		{
			continue;
		}

		const MeshDraw& draw = sceneData.meshInstances[drawIndex];
		draw.Geometry.MeshKind == RenderMeshKind::Skeletal
		    ? ++sceneData.meshWorkload.skinnedInstanceCount
		    : ++sceneData.meshWorkload.staticInstanceCount;
	}

	for (const MeshInstanceBatch& batch : sceneData.meshInstanceBatches)
	{
		batch.meshKind == RenderMeshKind::Skeletal
		    ? ++sceneData.meshWorkload.skinnedBatchCount
		    : ++sceneData.meshWorkload.staticBatchCount;
	}
}
