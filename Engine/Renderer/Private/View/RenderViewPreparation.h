#pragma once

#include "Core/Public/Math/Frustum.h"
#include "Tasks/Public/TaskGraph.h"
#include "Tasks/Public/TaskTypes.h"
#include "View/MeshInstanceBatchBuilder.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <vector>

class TaskExecutionContext;
class TaskExecutor;
struct PreparedRenderScene;
class RenderViewState;
struct RenderMeshWorldBounds;
struct RenderView;

class RenderViewPreparation final
{
public:
	explicit RenderViewPreparation(TaskExecutor& taskExecutor) noexcept;
	void Prepare(const PreparedRenderScene& preparedScene, RenderView& view, RenderViewState& viewState);

private:
	struct Run final
	{
		const PreparedRenderScene* Scene = nullptr;
		const RenderView* View = nullptr;
		std::vector<MeshRenderItem> Items;
	};

	static TaskResult EvaluateVisibility(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
	static bool Intersects(const Frustum& frustum, const RenderMeshWorldBounds& bounds) noexcept;
	static RenderMaterialClassification ClassifyMaterial(std::uint32_t alphaMode) noexcept;
	static float ComputeCameraDistanceSquared(
	    const DirectX::XMFLOAT3& cameraPosition,
	    const RenderMeshWorldBounds& bounds,
	    const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
	static void BuildWorkload(const PreparedRenderScene& scene, RenderView& view) noexcept;
	void EnsureGraph(std::size_t primitiveCount);

	TaskExecutor* m_taskExecutor = nullptr;
	Run m_run;
	std::vector<MeshRenderItem> m_visibleItems;
	MeshInstanceBatchBuilder m_batchBuilder;
	MeshInstanceBatchBuildResult m_batchResult;
	CompiledTaskGraph m_graph;
	std::uint32_t m_capacity = 0u;
};
