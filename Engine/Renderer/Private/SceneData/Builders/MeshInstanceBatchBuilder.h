#pragma once

#include "Renderer/Public/Meshes/MeshDiagnostics.h"
#include "Renderer/Public/SceneData/MeshDraw.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class GPUMesh;
class RenderBindingSet;

struct MeshRenderItem final
{
	MeshDraw draw;
	const RenderBindingSet* materialBindingSet = nullptr;
	RenderMeshInstanceGroupIndex instanceGroupIndex = kInvalidRenderMeshInstanceGroupIndex;
	RenderMeshInstanceGroupKind instanceGroupKind = RenderMeshInstanceGroupKind::None;
	std::uint32_t sourceInstanceIndex = 0;
	std::uint32_t renderStateKey = 0;
	std::uint32_t renderLayer = 0;
};

struct MeshInstanceBatchBuildOptions final
{
	bool enableAutoBatching = true;
	bool requireMaterialBindingSet = true;
	bool collectDiagnostics = false;
};

struct MeshInstanceBatchBuildResult final
{
	std::vector<MeshDraw> batchInstances;
	std::vector<MeshInstanceBatch> batches;
	MeshGeometryInstancingDiagnostics diagnostics;
};

class MeshInstanceBatchBuilder final
{
  public:
	MeshInstanceBatchBuildResult Build(
	    const std::vector<MeshRenderItem>& renderItems,
	    const std::vector<RenderMeshInstanceGroup>& instanceGroups,
	    const MeshInstanceBatchBuildOptions& options) const;

  private:
	struct BatchKey final
	{
		const GPUMesh* gpuMesh = nullptr;
		const RenderBindingSet* materialBindingSet = nullptr;
		std::uint32_t materialSlot = 0;
		std::uint64_t skeletonAssetId = 0;
		RenderMeshKind meshKind = RenderMeshKind::Static;
		std::uint32_t renderStateKey = 0;
		std::uint32_t renderLayer = 0;

		bool operator==(const BatchKey& other) const noexcept = default;
	};

	static bool IsValidCandidate(
	    const MeshRenderItem& item,
	    std::size_t instanceGroupCount,
	    const MeshInstanceBatchBuildOptions& options,
	    MeshGeometryInstancingDiagnostics& diagnostics) noexcept;
	static BatchKey MakeBatchKey(const MeshRenderItem& item) noexcept;
	static bool CanShareBatch(const MeshRenderItem& lhs, const MeshRenderItem& rhs) noexcept;
	static MeshInstanceBatchSource ResolvePreservedGroupSource(RenderMeshInstanceGroupKind groupKind) noexcept;
	static void AppendBatch(
	    const std::vector<MeshRenderItem>& renderItems,
	    const std::vector<std::size_t>& itemIndices,
	    MeshInstanceBatchSource source,
	    bool collectDiagnostics,
	    MeshInstanceBatchBuildResult& result);
};
