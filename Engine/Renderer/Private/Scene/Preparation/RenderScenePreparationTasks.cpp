#include "PCH.h"

#include "Scene/Preparation/RenderScenePreparationTasks.h"

#include "Scene/Preparation/RenderDeformationPreparation.h"
#include "Scene/Preparation/RenderLightPreparation.h"
#include "Scene/Preparation/RenderPrimitivePreparation.h"
#include "Scene/Preparation/RenderScenePreparationRun.h"
#include "Tasks/Public/TaskExecutionContext.h"

#include <algorithm>
#include <span>

TaskResult RenderScenePreparationTasks::TransformPrimitives(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context)
{
	RenderScenePreparationRun& run = *context.TryGet<RenderScenePreparationRun>();
	const std::size_t rangeBegin = (std::min<std::size_t>) (begin, run.ResolvedPrimitives.size());
	const std::size_t rangeEnd = (std::min<std::size_t>) (end, run.ResolvedPrimitives.size());
	RenderPrimitivePreparation::TransformRange(
	    std::span<const ResolvedRenderPrimitive>{run.ResolvedPrimitives}.subspan(rangeBegin, rangeEnd - rangeBegin),
	    std::span<PreparedRenderPrimitive>{run.PreparedPrimitives}.subspan(rangeBegin, rangeEnd - rangeBegin));
	return TaskResult::Success();
}

TaskResult RenderScenePreparationTasks::CopyJointMatrices(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context)
{
	RenderScenePreparationRun& run = *context.TryGet<RenderScenePreparationRun>();
	const std::size_t rangeBegin = (std::min<std::size_t>) (begin, run.Deformation.JointMatrixCopyRanges.size());
	const std::size_t rangeEnd = (std::min<std::size_t>) (end, run.Deformation.JointMatrixCopyRanges.size());
	RenderDeformationPreparation::CopyJointMatrixRanges(
	    std::span<const RenderJointMatrixCopyRange>{run.Deformation.JointMatrixCopyRanges}.subspan(rangeBegin, rangeEnd - rangeBegin),
	    run.Deformation.JointMatrices,
	    run.Deformation.PreviousJointMatrices);
	return TaskResult::Success();
}

TaskResult RenderScenePreparationTasks::CopyMorphWeights(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context)
{
	RenderScenePreparationRun& run = *context.TryGet<RenderScenePreparationRun>();
	const std::size_t rangeBegin = (std::min<std::size_t>) (begin, run.Deformation.MorphWeightCopyRanges.size());
	const std::size_t rangeEnd = (std::min<std::size_t>) (end, run.Deformation.MorphWeightCopyRanges.size());
	RenderDeformationPreparation::CopyMorphWeightRanges(
	    std::span<const RenderMorphWeightCopyRange>{run.Deformation.MorphWeightCopyRanges}.subspan(rangeBegin, rangeEnd - rangeBegin),
	    run.Deformation.MorphWeights,
	    run.Deformation.PreviousMorphWeights);
	return TaskResult::Success();
}

TaskResult RenderScenePreparationTasks::PrepareLights(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context)
{
	RenderScenePreparationRun& run = *context.TryGet<RenderScenePreparationRun>();
	const std::size_t rangeBegin = (std::min<std::size_t>) (begin, run.PreparedLights.size());
	const std::size_t rangeEnd = (std::min<std::size_t>) (end, run.PreparedLights.size());
	RenderLightPreparation::PrepareRange(
	    run.Lights.subspan(rangeBegin, rangeEnd - rangeBegin),
	    std::span<PreparedRenderLight>{run.PreparedLights}.subspan(rangeBegin, rangeEnd - rangeBegin));
	return TaskResult::Success();
}
