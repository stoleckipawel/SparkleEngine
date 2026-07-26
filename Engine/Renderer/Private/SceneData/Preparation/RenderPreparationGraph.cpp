#include "PCH.h"

#include "SceneData/Preparation/RenderPreparationGraph.h"

#include "SceneData/Preparation/RenderDeformationPreparation.h"
#include "SceneData/Preparation/RenderPreparationInputResolver.h"
#include "SceneData/Preparation/RenderPreparationMerger.h"
#include "SceneData/Preparation/RenderPreparationRun.h"
#include "SceneData/Preparation/RenderPreparationTasks.h"
#include "SceneData/RenderSceneData.h"
#include "Tasks/Public/ParallelFor.h"
#include "Tasks/Public/TaskExecution.h"
#include "Tasks/Public/TaskExecutionContext.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskGraph.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

struct RenderPreparationGraph::Impl final
{
	struct Capacity final
	{
		std::uint32_t Objects = 0u;
		std::uint32_t Lights = 0u;
		std::uint32_t Skinning = 0u;
		std::uint32_t Morph = 0u;
	};

	Impl(
	    TaskExecutor& taskExecutor,
	    MaterialCacheManager& materialCache,
	    GPUMeshCache& gpuMeshCache,
	    TextureManager& textureManager) noexcept;

	RenderSceneData Execute(
	    const RenderWorld& world,
	    const RenderFrameDynamicData& dynamic,
	    const Frustum& frustum);
	void ResetHistory() noexcept;

  private:
	void ResolveInputs(
	    const RenderWorld& world,
	    const RenderFrameDynamicData& dynamic,
	    const Frustum& frustum);
	bool ExecuteCompiledGraph();
	RenderSceneData FinishRun() noexcept;
	void EnsureGraph(
	    std::size_t objectCount,
	    std::size_t lightCount,
	    std::size_t skinningCount,
	    std::size_t morphCount);
	bool CanReuseGraph(const Capacity& capacity) const noexcept;
	static Capacity ResolveCapacities(
	    std::size_t objectCount,
	    std::size_t lightCount,
	    std::size_t skinningCount,
	    std::size_t morphCount) noexcept;
	static CompiledTaskGraph CompileGraph(const Capacity& capacity);
	static TaskNodeHandle AddTransformBoundsTasks(TaskGraphBuilder& builder, std::uint32_t capacity);
	static TaskNodeHandle AddVisibilityTasks(
	    TaskGraphBuilder& builder,
	    std::uint32_t capacity,
	    TaskNodeHandle transform);
	static TaskNodeHandle AddSkinningTasks(TaskGraphBuilder& builder, std::uint32_t capacity);
	static TaskNodeHandle AddMorphTasks(TaskGraphBuilder& builder, std::uint32_t capacity);
	static TaskNodeHandle AddLightingTasks(TaskGraphBuilder& builder, std::uint32_t capacity);
	static void AddMergeTasks(
	    TaskGraphBuilder& builder,
	    TaskNodeHandle visibility,
	    TaskNodeHandle skinning,
	    TaskNodeHandle morph,
	    TaskNodeHandle lighting);
	static TaskDesc MakeTaskDesc(std::string_view name);
	void CommitHistory();
	static void ReleaseInputViews(RenderPreparationRun& run) noexcept;
	static std::uint32_t ResolveCapacity(std::size_t count, std::uint32_t serialThreshold) noexcept;

	TaskExecutor* m_taskExecutor = nullptr;
	RenderPreparationInputResolver m_inputResolver;
	RenderDeformationPreparation m_deformationPreparation;
	RenderPreparationRun m_run;
	std::vector<RenderPreviousWorldTransform> m_previousWorldTransforms;
	CompiledTaskGraph m_graph;
	Capacity m_capacity;
};

RenderPreparationGraph::Impl::Impl(
    TaskExecutor& taskExecutor,
    MaterialCacheManager& materialCache,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager) noexcept :
	m_taskExecutor(&taskExecutor),
	m_inputResolver(materialCache, gpuMeshCache, textureManager)
{
}

RenderSceneData RenderPreparationGraph::Impl::Execute(
    const RenderWorld& world,
    const RenderFrameDynamicData& dynamic,
    const Frustum& frustum)
{
	ResolveInputs(world, dynamic, frustum);
	EnsureGraph(
	    m_run.ResolvedObjects.size(),
	    m_run.PreparedLights.size(),
	    m_run.Deformation.SkinningRanges.size(),
	    m_run.Deformation.MorphRanges.size());

	if (!ExecuteCompiledGraph())
	{
		ResetHistory();
		return FinishRun();
	}

	CommitHistory();
	return FinishRun();
}

void RenderPreparationGraph::Impl::ResolveInputs(
    const RenderWorld& world,
    const RenderFrameDynamicData& dynamic,
    const Frustum& frustum)
{
	m_run.SceneData = {};
	m_inputResolver.Resolve(
	    world,
	    dynamic,
	    frustum,
	    m_previousWorldTransforms,
	    m_deformationPreparation,
	    m_run);
}

bool RenderPreparationGraph::Impl::ExecuteCompiledGraph()
{
	if (!m_graph)
	{
		return false;
	}

	TaskExecutionContext context(m_run);
	const TaskExecution execution = m_taskExecutor->Submit(m_graph, context);
	return execution.GetStatus() == TaskExecutionStatus::Succeeded;
}

RenderSceneData RenderPreparationGraph::Impl::FinishRun() noexcept
{
	ReleaseInputViews(m_run);
	return std::move(m_run.SceneData);
}

void RenderPreparationGraph::Impl::ResetHistory() noexcept
{
	m_previousWorldTransforms.clear();
	m_deformationPreparation.Reset();
}

void RenderPreparationGraph::Impl::EnsureGraph(
    std::size_t objectCount,
    std::size_t lightCount,
    std::size_t skinningCount,
    std::size_t morphCount)
{
	const Capacity capacity = ResolveCapacities(objectCount, lightCount, skinningCount, morphCount);
	if (CanReuseGraph(capacity))
	{
		return;
	}

	m_graph = CompileGraph(capacity);
	m_capacity = capacity;
}

bool RenderPreparationGraph::Impl::CanReuseGraph(const Capacity& capacity) const noexcept
{
	return m_graph &&
	       capacity.Objects == m_capacity.Objects &&
	       capacity.Lights == m_capacity.Lights &&
	       capacity.Skinning == m_capacity.Skinning &&
	       capacity.Morph == m_capacity.Morph;
}

RenderPreparationGraph::Impl::Capacity RenderPreparationGraph::Impl::ResolveCapacities(
    std::size_t objectCount,
    std::size_t lightCount,
    std::size_t skinningCount,
    std::size_t morphCount) noexcept
{
	return Capacity{
	    .Objects = ResolveCapacity(objectCount, 128u),
	    .Lights = ResolveCapacity(lightCount, 32u),
	    .Skinning = ResolveCapacity(skinningCount, 64u),
	    .Morph = ResolveCapacity(morphCount, 64u)};
}

CompiledTaskGraph RenderPreparationGraph::Impl::CompileGraph(const Capacity& capacity)
{
	TaskGraphBuilder builder;

	const TaskNodeHandle transform = AddTransformBoundsTasks(builder, capacity.Objects);
	const TaskNodeHandle visibility = AddVisibilityTasks(builder, capacity.Objects, transform);
	const TaskNodeHandle skinning = AddSkinningTasks(builder, capacity.Skinning);
	const TaskNodeHandle morph = AddMorphTasks(builder, capacity.Morph);
	const TaskNodeHandle lighting = AddLightingTasks(builder, capacity.Lights);

	AddMergeTasks(builder, visibility, skinning, morph, lighting);
	return builder.Compile();
}

TaskNodeHandle RenderPreparationGraph::Impl::AddTransformBoundsTasks(
    TaskGraphBuilder& builder,
    std::uint32_t capacity)
{
	return ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Preparation.TransformBounds"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 64u, .SerialThreshold = 128u, .MaximumPartitions = 8u},
	    &RenderPreparationTasks::TransformObjects);
}

TaskNodeHandle RenderPreparationGraph::Impl::AddVisibilityTasks(
    TaskGraphBuilder& builder,
    std::uint32_t capacity,
    TaskNodeHandle transform)
{
	const TaskNodeHandle visibility = ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Preparation.Visibility"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 64u, .SerialThreshold = 128u, .MaximumPartitions = 8u},
	    &RenderPreparationTasks::EvaluateVisibility);

	(void)builder.DependsOn(visibility, transform);
	return visibility;
}

TaskNodeHandle RenderPreparationGraph::Impl::AddSkinningTasks(
    TaskGraphBuilder& builder,
    std::uint32_t capacity)
{
	return ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Preparation.Skinning"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 16u, .SerialThreshold = 64u, .MaximumPartitions = 8u},
	    &RenderPreparationTasks::CopySkinning);
}

TaskNodeHandle RenderPreparationGraph::Impl::AddMorphTasks(
    TaskGraphBuilder& builder,
    std::uint32_t capacity)
{
	return ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Preparation.Morph"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 16u, .SerialThreshold = 64u, .MaximumPartitions = 8u},
	    &RenderPreparationTasks::CopyMorph);
}

TaskNodeHandle RenderPreparationGraph::Impl::AddLightingTasks(
    TaskGraphBuilder& builder,
    std::uint32_t capacity)
{
	return ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Preparation.Lighting"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 16u, .SerialThreshold = 32u, .MaximumPartitions = 4u},
	    &RenderPreparationTasks::PrepareLights);
}

void RenderPreparationGraph::Impl::AddMergeTasks(
    TaskGraphBuilder& builder,
    TaskNodeHandle visibility,
    TaskNodeHandle skinning,
    TaskNodeHandle morph,
    TaskNodeHandle lighting)
{
	const std::array<TaskNodeHandle, 4> mergeInputs{visibility, skinning, morph, lighting};
	const TaskNodeHandle mergeJoin = builder.WhenAll(
	    MakeTaskDesc("Renderer.Preparation.MergeJoin"),
	    mergeInputs);
	const TaskNodeHandle merge = builder.ContinueWith(
	    mergeJoin,
	    MakeTaskDesc("Renderer.Preparation.Merge"),
	    &RenderPreparationMerger::Merge);

	(void)builder.ContinueWith(
	    merge,
	    MakeTaskDesc("Renderer.Preparation.RayTracingPlan"),
	    &RenderPreparationMerger::BuildRayTracingPlan);
}

TaskDesc RenderPreparationGraph::Impl::MakeTaskDesc(std::string_view name)
{
	return TaskDesc{
	    .Name = TaskName(name),
	    .Lane = TaskLane::FrameCritical};
}

void RenderPreparationGraph::Impl::CommitHistory()
{
	m_previousWorldTransforms.clear();
	m_previousWorldTransforms.reserve(m_run.PreparedObjects.size());

	for (const PreparedRenderObject& object : m_run.PreparedObjects)
	{
		if (!object.Object.IsValid())
		{
			continue;
		}

		m_previousWorldTransforms.push_back(
		    RenderPreviousWorldTransform{
		        .Object = object.Object,
		        .WorldMatrix = object.Draw.Transform.WorldMatrix});
	}

	m_deformationPreparation.Commit(m_run.Deformation);
}

void RenderPreparationGraph::Impl::ReleaseInputViews(RenderPreparationRun& run) noexcept
{
	run.Lights = {};
	run.Deformation.SkinningRanges.clear();
	run.Deformation.MorphRanges.clear();
}

std::uint32_t RenderPreparationGraph::Impl::ResolveCapacity(
    std::size_t count,
    std::uint32_t serialThreshold) noexcept
{
	const std::size_t maximumCapacity = (std::numeric_limits<std::uint32_t>::max)();
	const std::uint32_t boundedCount = static_cast<std::uint32_t>((std::min)(count, maximumCapacity));

	return boundedCount <= serialThreshold
	           ? serialThreshold
	           : std::bit_ceil(boundedCount);
}

RenderPreparationGraph::RenderPreparationGraph(
    TaskExecutor& taskExecutor,
    MaterialCacheManager& materialCache,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager) :
	m_impl(std::make_unique<Impl>(taskExecutor, materialCache, gpuMeshCache, textureManager))
{
}

RenderPreparationGraph::~RenderPreparationGraph() noexcept = default;

RenderSceneData RenderPreparationGraph::Execute(
    const RenderWorld& world,
    const RenderFrameDynamicData& dynamic,
    const Frustum& frustum)
{
	return m_impl->Execute(world, dynamic, frustum);
}

void RenderPreparationGraph::ResetHistory() noexcept
{
	m_impl->ResetHistory();
}
