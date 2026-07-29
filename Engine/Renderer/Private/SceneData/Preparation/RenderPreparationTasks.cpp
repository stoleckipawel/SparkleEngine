#include "PCH.h"

#include "SceneData/Preparation/RenderPreparationTasks.h"

#include "SceneData/Preparation/RenderDeformationPreparation.h"
#include "SceneData/Preparation/RenderLightPreparation.h"
#include "SceneData/Preparation/RenderObjectPreparation.h"
#include "SceneData/Preparation/RenderPreparationRun.h"
#include "Tasks/Public/TaskExecutionContext.h"

#include <algorithm>
#include <span>

TaskResult RenderPreparationTasks::TransformObjects(
    std::uint32_t begin,
    std::uint32_t end,
    TaskExecutionContext& context)
{
	RenderPreparationRun& run =
	    *context.TryGet<RenderPreparationRun>();
	const std::size_t rangeBegin =
	    (std::min<std::size_t>)(
	        begin,
	        run.ResolvedObjects.size());
	const std::size_t rangeEnd =
	    (std::min<std::size_t>)(
	        end,
	        run.ResolvedObjects.size());
	RenderObjectPreparation::TransformRange(
	    std::span<const ResolvedRenderObject>{
	        run.ResolvedObjects}
	        .subspan(
	            rangeBegin,
	            rangeEnd - rangeBegin),
	    std::span<PreparedRenderObject>{
	        run.PreparedObjects}
	        .subspan(
	            rangeBegin,
	            rangeEnd - rangeBegin));
	return TaskResult::Success();
}

TaskResult RenderPreparationTasks::EvaluateVisibility(
    std::uint32_t begin,
    std::uint32_t end,
    TaskExecutionContext& context)
{
	RenderPreparationRun& run =
	    *context.TryGet<RenderPreparationRun>();
	const std::size_t rangeBegin =
	    (std::min<std::size_t>)(
	        begin,
	        run.PreparedObjects.size());
	const std::size_t rangeEnd =
	    (std::min<std::size_t>)(
	        end,
	        run.PreparedObjects.size());
	RenderObjectPreparation::EvaluateVisibilityRange(
	    run.ViewFrustum,
	    run.CameraPosition,
	    std::span<PreparedRenderObject>{
	        run.PreparedObjects}
	        .subspan(
	            rangeBegin,
	            rangeEnd - rangeBegin));
	return TaskResult::Success();
}

TaskResult RenderPreparationTasks::CopyJointMatrices(
    std::uint32_t begin,
    std::uint32_t end,
    TaskExecutionContext& context)
{
	RenderPreparationRun& run =
	    *context.TryGet<RenderPreparationRun>();
	const std::size_t rangeBegin =
	    (std::min<std::size_t>)(
	        begin,
	        run.Deformation.JointMatrixCopyRanges.size());
	const std::size_t rangeEnd =
	    (std::min<std::size_t>)(
	        end,
	        run.Deformation.JointMatrixCopyRanges.size());
	RenderDeformationPreparation::CopyJointMatrixRanges(
	    std::span<const RenderJointMatrixCopyRange>{
	        run.Deformation.JointMatrixCopyRanges}
	        .subspan(
	            rangeBegin,
	            rangeEnd - rangeBegin),
	    run.Deformation.JointMatrices,
	    run.Deformation.PreviousJointMatrices);
	return TaskResult::Success();
}

TaskResult RenderPreparationTasks::CopyMorphWeights(
    std::uint32_t begin,
    std::uint32_t end,
    TaskExecutionContext& context)
{
	RenderPreparationRun& run =
	    *context.TryGet<RenderPreparationRun>();
	const std::size_t rangeBegin =
	    (std::min<std::size_t>)(
	        begin,
	        run.Deformation.MorphWeightCopyRanges.size());
	const std::size_t rangeEnd =
	    (std::min<std::size_t>)(
	        end,
	        run.Deformation.MorphWeightCopyRanges.size());
	RenderDeformationPreparation::CopyMorphWeightRanges(
	    std::span<const RenderMorphWeightCopyRange>{
	        run.Deformation.MorphWeightCopyRanges}
	        .subspan(
	            rangeBegin,
	            rangeEnd - rangeBegin),
	    run.Deformation.MorphWeights,
	    run.Deformation.PreviousMorphWeights);
	return TaskResult::Success();
}

TaskResult RenderPreparationTasks::PrepareLights(
    std::uint32_t begin,
    std::uint32_t end,
    TaskExecutionContext& context)
{
	RenderPreparationRun& run =
	    *context.TryGet<RenderPreparationRun>();
	const std::size_t rangeBegin =
	    (std::min<std::size_t>)(
	        begin,
	        run.PreparedLights.size());
	const std::size_t rangeEnd =
	    (std::min<std::size_t>)(
	        end,
	        run.PreparedLights.size());
	RenderLightPreparation::PrepareRange(
	    run.Lights.subspan(
	        rangeBegin,
	        rangeEnd - rangeBegin),
	    std::span<PreparedRenderLight>{
	        run.PreparedLights}
	        .subspan(
	            rangeBegin,
	            rangeEnd - rangeBegin));
	return TaskResult::Success();
}
