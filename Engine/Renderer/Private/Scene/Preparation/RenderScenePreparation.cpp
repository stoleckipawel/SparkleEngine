#include "PCH.h"

#include "Scene/Preparation/RenderScenePreparation.h"

#include "Scene/RenderScene.h"
#include "Scene/Preparation/RenderDeformationPreparation.h"
#include "Scene/Preparation/RenderScenePreparationInputResolver.h"
#include "Scene/Preparation/RenderScenePreparationMerger.h"
#include "Scene/Preparation/RenderScenePreparationRun.h"
#include "Scene/Preparation/RenderScenePreparationTasks.h"
#include "Scene/Preparation/PreparedRenderScene.h"
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

struct RenderScenePreparation::Impl final
{
	struct Capacity final
	{
		std::uint32_t Primitives = 0u;
		std::uint32_t Lights = 0u;
		std::uint32_t JointMatrixCopyRanges = 0u;
		std::uint32_t MorphWeightCopyRanges = 0u;
	};

	Impl(TaskExecutor& taskExecutor, GpuMeshCache& gpuMeshCache, TextureCache& textureCache) noexcept;

	void Execute(RenderScene& scene, PreparedRenderScene& output);

private:
	void BeginRun(PreparedRenderScene& output);
	void ResolveInputs(RenderScene& scene);
	bool ExecuteCompiledGraph();
	void FinishRun(PreparedRenderScene& output) noexcept;
	void EnsureGraph(
	    std::size_t primitiveCount,
	    std::size_t lightCount,
	    std::size_t jointMatrixCopyRangeCount,
	    std::size_t morphWeightCopyRangeCount);
	bool CanReuseGraph(const Capacity& capacity) const noexcept;
	static Capacity ResolveCapacities(
	    std::size_t primitiveCount,
	    std::size_t lightCount,
	    std::size_t jointMatrixCopyRangeCount,
	    std::size_t morphWeightCopyRangeCount) noexcept;
	static CompiledTaskGraph CompileGraph(const Capacity& capacity);
	static TaskNodeHandle AddTransformBoundsTasks(TaskGraphBuilder& builder, std::uint32_t capacity);
	static TaskNodeHandle AddJointMatrixTasks(TaskGraphBuilder& builder, std::uint32_t capacity);
	static TaskNodeHandle AddMorphWeightTasks(TaskGraphBuilder& builder, std::uint32_t capacity);
	static TaskNodeHandle AddLightingTasks(TaskGraphBuilder& builder, std::uint32_t capacity);
	static void AddMergeTasks(
	    TaskGraphBuilder& builder,
	    TaskNodeHandle primitives,
	    TaskNodeHandle jointMatrices,
	    TaskNodeHandle morphWeights,
	    TaskNodeHandle lighting);
	static TaskDesc MakeTaskDesc(std::string_view name);
	void CommitHistory(RenderScene& scene);
	static void ReleaseInputViews(RenderScenePreparationRun& run) noexcept;
	static std::uint32_t ResolveCapacity(std::size_t count, std::uint32_t serialThreshold) noexcept;

	TaskExecutor* m_taskExecutor = nullptr;
	RenderScenePreparationInputResolver m_inputResolver;
	RenderDeformationPreparation m_deformationPreparation;
	RenderScenePreparationRun m_run;
	CompiledTaskGraph m_graph;
	Capacity m_capacity;
};

RenderScenePreparation::Impl::Impl(TaskExecutor& taskExecutor, GpuMeshCache& gpuMeshCache, TextureCache& textureCache) noexcept :
    m_taskExecutor(&taskExecutor),
    m_inputResolver(gpuMeshCache, textureCache)
{
}

void RenderScenePreparation::Impl::Execute(RenderScene& scene, PreparedRenderScene& output)
{
	BeginRun(output);
	ResolveInputs(scene);
	EnsureGraph(
	    m_run.ResolvedPrimitives.size(),
	    m_run.PreparedLights.size(),
	    m_run.Deformation.JointMatrixCopyRanges.size(),
	    m_run.Deformation.MorphWeightCopyRanges.size());

	if (!ExecuteCompiledGraph())
	{
		scene.ResetContinuity();
		m_run.PreparedScene.ResetForReuse();
		FinishRun(output);
		return;
	}

	CommitHistory(scene);
	RenderScenePreparationMerger::PublishSceneOutputs(m_run);
	FinishRun(output);
}

void RenderScenePreparation::Impl::BeginRun(PreparedRenderScene& output)
{
	m_run.PreparedScene = std::move(output);
	m_run.Deformation.JointMatrices = std::move(m_run.PreparedScene.jointMatrices);
	m_run.Deformation.PreviousJointMatrices = std::move(m_run.PreparedScene.previousJointMatrices);
	m_run.Deformation.MorphWeights = std::move(m_run.PreparedScene.morphWeights);
	m_run.Deformation.PreviousMorphWeights = std::move(m_run.PreparedScene.previousMorphWeights);
	m_run.PreparedScene.ResetForReuse();
}

void RenderScenePreparation::Impl::ResolveInputs(RenderScene& scene)
{
	m_inputResolver.Resolve(scene, m_deformationPreparation, m_run);
}

bool RenderScenePreparation::Impl::ExecuteCompiledGraph()
{
	if (!m_graph)
	{
		return false;
	}

	TaskExecutionContext context(m_run);
	const TaskExecution execution = m_taskExecutor->Submit(m_graph, context);
	return execution.GetStatus() == TaskExecutionStatus::Succeeded;
}

void RenderScenePreparation::Impl::FinishRun(PreparedRenderScene& output) noexcept
{
	ReleaseInputViews(m_run);
	output = std::move(m_run.PreparedScene);
}

void RenderScenePreparation::Impl::EnsureGraph(
    std::size_t primitiveCount,
    std::size_t lightCount,
    std::size_t jointMatrixCopyRangeCount,
    std::size_t morphWeightCopyRangeCount)
{
	const Capacity capacity = ResolveCapacities(primitiveCount, lightCount, jointMatrixCopyRangeCount, morphWeightCopyRangeCount);
	if (CanReuseGraph(capacity))
	{
		return;
	}

	m_graph = CompileGraph(capacity);
	m_capacity = capacity;
}

bool RenderScenePreparation::Impl::CanReuseGraph(const Capacity& capacity) const noexcept
{
	return m_graph && capacity.Primitives == m_capacity.Primitives && capacity.Lights == m_capacity.Lights
	    && capacity.JointMatrixCopyRanges == m_capacity.JointMatrixCopyRanges
	    && capacity.MorphWeightCopyRanges == m_capacity.MorphWeightCopyRanges;
}

RenderScenePreparation::Impl::Capacity RenderScenePreparation::Impl::ResolveCapacities(
    std::size_t primitiveCount,
    std::size_t lightCount,
    std::size_t jointMatrixCopyRangeCount,
    std::size_t morphWeightCopyRangeCount) noexcept
{
	return Capacity{
	    .Primitives = ResolveCapacity(primitiveCount, 128u),
	    .Lights = ResolveCapacity(lightCount, 32u),
	    .JointMatrixCopyRanges = ResolveCapacity(jointMatrixCopyRangeCount, 64u),
	    .MorphWeightCopyRanges = ResolveCapacity(morphWeightCopyRangeCount, 64u)};
}

CompiledTaskGraph RenderScenePreparation::Impl::CompileGraph(const Capacity& capacity)
{
	TaskGraphBuilder builder;

	const TaskNodeHandle primitives = AddTransformBoundsTasks(builder, capacity.Primitives);
	const TaskNodeHandle jointMatrices = AddJointMatrixTasks(builder, capacity.JointMatrixCopyRanges);
	const TaskNodeHandle morphWeights = AddMorphWeightTasks(builder, capacity.MorphWeightCopyRanges);
	const TaskNodeHandle lighting = AddLightingTasks(builder, capacity.Lights);

	AddMergeTasks(builder, primitives, jointMatrices, morphWeights, lighting);
	return builder.Compile();
}

TaskNodeHandle RenderScenePreparation::Impl::AddTransformBoundsTasks(TaskGraphBuilder& builder, std::uint32_t capacity)
{
	return ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Scene.Preparation.TransformBounds"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 64u, .SerialThreshold = 128u, .MaximumPartitions = 8u},
	    &RenderScenePreparationTasks::TransformPrimitives);
}

TaskNodeHandle RenderScenePreparation::Impl::AddJointMatrixTasks(TaskGraphBuilder& builder, std::uint32_t capacity)
{
	return ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Scene.Preparation.JointMatrices"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 16u, .SerialThreshold = 64u, .MaximumPartitions = 8u},
	    &RenderScenePreparationTasks::CopyJointMatrices);
}

TaskNodeHandle RenderScenePreparation::Impl::AddMorphWeightTasks(TaskGraphBuilder& builder, std::uint32_t capacity)
{
	return ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Scene.Preparation.MorphWeights"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 16u, .SerialThreshold = 64u, .MaximumPartitions = 8u},
	    &RenderScenePreparationTasks::CopyMorphWeights);
}

TaskNodeHandle RenderScenePreparation::Impl::AddLightingTasks(TaskGraphBuilder& builder, std::uint32_t capacity)
{
	return ParallelFor(
	    builder,
	    MakeTaskDesc("Renderer.Scene.Preparation.Lighting"),
	    capacity,
	    ParallelForPolicy{.GrainSize = 16u, .SerialThreshold = 32u, .MaximumPartitions = 4u},
	    &RenderScenePreparationTasks::PrepareLights);
}

void RenderScenePreparation::Impl::AddMergeTasks(
    TaskGraphBuilder& builder,
    TaskNodeHandle primitives,
    TaskNodeHandle jointMatrices,
    TaskNodeHandle morphWeights,
    TaskNodeHandle lighting)
{
	const std::array<TaskNodeHandle, 4> mergeInputs{primitives, jointMatrices, morphWeights, lighting};
	const TaskNodeHandle mergeJoin = builder.WhenAll(MakeTaskDesc("Renderer.Scene.Preparation.MergeJoin"), mergeInputs);
	const TaskNodeHandle merge =
	    builder.ContinueWith(mergeJoin, MakeTaskDesc("Renderer.Scene.Preparation.Merge"), &RenderScenePreparationMerger::Merge);

	(void) builder.ContinueWith(
	    merge,
	    MakeTaskDesc("Renderer.Scene.Preparation.RayTracingPlan"),
	    &RenderScenePreparationMerger::BuildRayTracingPlan);
}

TaskDesc RenderScenePreparation::Impl::MakeTaskDesc(std::string_view name)
{
	return TaskDesc{.Name = TaskName(name), .Lane = TaskLane::FrameCritical};
}

void RenderScenePreparation::Impl::CommitHistory(RenderScene& scene)
{
	scene.CommitContinuity(m_run.PreparedPrimitives, m_run.Deformation);
}

void RenderScenePreparation::Impl::ReleaseInputViews(RenderScenePreparationRun& run) noexcept
{
	run.Lights = {};
	run.Deformation.JointMatrixCopyRanges.clear();
	run.Deformation.MorphWeightCopyRanges.clear();
}

std::uint32_t RenderScenePreparation::Impl::ResolveCapacity(std::size_t count, std::uint32_t serialThreshold) noexcept
{
	const std::size_t maximumCapacity = (std::numeric_limits<std::uint32_t>::max)();
	const std::uint32_t boundedCount = static_cast<std::uint32_t>((std::min) (count, maximumCapacity));

	return boundedCount <= serialThreshold ? serialThreshold : std::bit_ceil(boundedCount);
}

RenderScenePreparation::RenderScenePreparation(TaskExecutor& taskExecutor, GpuMeshCache& gpuMeshCache, TextureCache& textureCache) :
    m_impl(std::make_unique<Impl>(taskExecutor, gpuMeshCache, textureCache))
{
}

RenderScenePreparation::~RenderScenePreparation() noexcept = default;

void RenderScenePreparation::Execute(RenderScene& scene, PreparedRenderScene& output)
{
	m_impl->Execute(scene, output);
}
