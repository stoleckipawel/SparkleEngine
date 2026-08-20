#include "PCH.h"

#include "View/RenderViewPreparation.h"

#include "Renderer/Public/Debug/RendererCVars.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RenderScene.h"
#include "Tasks/Public/ParallelFor.h"
#include "Tasks/Public/TaskExecution.h"
#include "Tasks/Public/TaskExecutionContext.h"
#include "Tasks/Public/TaskExecutor.h"
#include "View/RenderView.h"

#include <algorithm>
#include <bit>
#include <limits>
#include <span>

RenderViewPreparation::RenderViewPreparation(TaskExecutor& taskExecutor) noexcept :
    m_taskExecutor(&taskExecutor)
{
}

void RenderViewPreparation::Prepare(const PreparedRenderScene& preparedScene, RenderView& view, RenderScene& renderScene)
{
	m_run.Scene = &preparedScene;
	m_run.View = &view;
	m_run.Items.resize(preparedScene.primitives.size());
	EnsureGraph(preparedScene.primitives.size());

	TaskExecutionContext context(m_run);
	const TaskExecution execution = m_taskExecutor->Submit(m_graph, context);
	if (execution.GetStatus() != TaskExecutionStatus::Succeeded)
	{
		view.rasterPrimitiveIndices.clear();
		view.meshInstanceBatches.clear();
		view.workload = {};
		m_run.Scene = nullptr;
		m_run.View = nullptr;
		return;
	}

	m_visibleItems.clear();
	m_visibleItems.reserve(m_run.Items.size());
	for (const MeshRenderItem& item : m_run.Items)
	{
		if (item.Classification != RenderMaterialClassification::Rejected)
		{
			m_visibleItems.push_back(item);
		}
	}

	m_batchResult.RasterInstanceIndices = std::move(view.rasterPrimitiveIndices);
	m_batchResult.Batches = std::move(view.meshInstanceBatches);
	m_batchBuilder.Build(
	    m_visibleItems,
	    preparedScene.primitives,
	    preparedScene.instanceGroups,
	    MeshInstanceBatchBuildOptions{
	        .EnableAutoBatching = CVarRendererMeshAutoBatching.Get(),
	        .RequireMaterialBindingSet = true,
	        .CollectDiagnostics = false},
	    m_batchResult);
	view.rasterPrimitiveIndices = std::move(m_batchResult.RasterInstanceIndices);
	view.meshInstanceBatches = std::move(m_batchResult.Batches);
	BuildWorkload(preparedScene, view);
	renderScene.PlanRayTracingFrame(preparedScene, view.cameraUniform.Position);

	m_run.Scene = nullptr;
	m_run.View = nullptr;
}

TaskResult RenderViewPreparation::EvaluateVisibility(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context)
{
	Run& run = *context.TryGet<Run>();
	const std::size_t rangeBegin = (std::min<std::size_t>) (begin, run.Scene->primitives.size());
	const std::size_t rangeEnd = (std::min<std::size_t>) (end, run.Scene->primitives.size());
	for (std::size_t primitiveIndex = rangeBegin; primitiveIndex < rangeEnd; ++primitiveIndex)
	{
		const PreparedRenderPrimitive& primitive = run.Scene->primitives[primitiveIndex];
		const RenderMaterialClassification classification = ClassifyMaterial(primitive.MaterialAlphaMode);
		const bool visible =
		    classification != RenderMaterialClassification::Rejected && Intersects(run.View->frustum, primitive.WorldBounds);
		run.Items[primitiveIndex] = visible
		    ? MeshRenderItem{
		          .Object = primitive.Object,
		          .DrawIndex = static_cast<std::uint32_t>(primitiveIndex),
		          .Material = primitive.Material,
		          .InstanceGroupIndex = primitive.InstanceGroupIndex,
		          .Classification = classification,
		          .CameraDistanceSquared = ComputeCameraDistanceSquared(
		              run.View->cameraUniform.Position,
		              primitive.WorldBounds,
		              primitive.Draw.Transform.WorldMatrix)}
		    : MeshRenderItem{};
	}
	return TaskResult::Success();
}

bool RenderViewPreparation::Intersects(const Frustum& frustum, const RenderMeshWorldBounds& bounds) noexcept
{
	if (!bounds.Valid)
	{
		return true;
	}

	for (const DirectX::XMFLOAT4& plane : frustum.planes)
	{
		const DirectX::XMFLOAT3 positive{
		    plane.x >= 0.0f ? bounds.Max.x : bounds.Min.x,
		    plane.y >= 0.0f ? bounds.Max.y : bounds.Min.y,
		    plane.z >= 0.0f ? bounds.Max.z : bounds.Min.z};
		if (plane.x * positive.x + plane.y * positive.y + plane.z * positive.z + plane.w < 0.0f)
		{
			return false;
		}
	}
	return true;
}

RenderMaterialClassification RenderViewPreparation::ClassifyMaterial(std::uint32_t alphaMode) noexcept
{
	switch (alphaMode)
	{
		case 0u:
			return RenderMaterialClassification::Opaque;
		case 1u:
			return RenderMaterialClassification::AlphaTested;
		case 2u:
			return RenderMaterialClassification::Transparent;
		default:
			return RenderMaterialClassification::Rejected;
	}
}

float RenderViewPreparation::ComputeCameraDistanceSquared(
    const DirectX::XMFLOAT3& cameraPosition,
    const RenderMeshWorldBounds& bounds,
    const DirectX::XMFLOAT4X4& worldMatrix) noexcept
{
	const DirectX::XMFLOAT3 center = bounds.Valid
	    ? DirectX::
	          XMFLOAT3{0.5f * (bounds.Min.x + bounds.Max.x), 0.5f * (bounds.Min.y + bounds.Max.y), 0.5f * (bounds.Min.z + bounds.Max.z)}
	    : DirectX::XMFLOAT3{worldMatrix._41, worldMatrix._42, worldMatrix._43};
	const float x = center.x - cameraPosition.x;
	const float y = center.y - cameraPosition.y;
	const float z = center.z - cameraPosition.z;
	return x * x + y * y + z * z;
}

void RenderViewPreparation::BuildWorkload(const PreparedRenderScene& scene, RenderView& view) noexcept
{
	view.workload = {};
	view.workload.jointMatrixCount = static_cast<std::uint32_t>(scene.jointMatrices.size());
	for (std::uint32_t primitiveIndex : view.rasterPrimitiveIndices)
	{
		if (primitiveIndex >= scene.primitives.size())
		{
			continue;
		}
		scene.primitives[primitiveIndex].Draw.Geometry.MeshKind == RenderMeshKind::Skeletal ? ++view.workload.skinnedInstanceCount
		                                                                                    : ++view.workload.staticInstanceCount;
	}
	for (const MeshInstanceBatch& batch : view.meshInstanceBatches)
	{
		batch.meshKind == RenderMeshKind::Skeletal ? ++view.workload.skinnedBatchCount : ++view.workload.staticBatchCount;
	}
}

void RenderViewPreparation::EnsureGraph(std::size_t primitiveCount)
{
	const std::size_t maximumCapacity = (std::numeric_limits<std::uint32_t>::max)();
	const std::uint32_t boundedCount = static_cast<std::uint32_t>((std::min) (primitiveCount, maximumCapacity));
	const std::uint32_t capacity = boundedCount <= 128u ? 128u : std::bit_ceil(boundedCount);
	if (m_graph && capacity == m_capacity)
	{
		return;
	}

	TaskGraphBuilder builder;
	(void) ParallelFor(
	    builder,
	    TaskDesc{.Name = TaskName("Renderer.View.Visibility"), .Lane = TaskLane::FrameCritical},
	    capacity,
	    ParallelForPolicy{.GrainSize = 64u, .SerialThreshold = 128u, .MaximumPartitions = 8u},
	    &RenderViewPreparation::EvaluateVisibility);
	m_graph = builder.Compile();
	m_capacity = capacity;
}
