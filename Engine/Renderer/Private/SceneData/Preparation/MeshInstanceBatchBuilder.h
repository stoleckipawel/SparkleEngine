#pragma once

#include "Renderer/Public/Meshes/MeshDiagnostics.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Rendering/RenderObjectId.h"
#include "SceneData/MaterialData.h"
#include "SceneData/Preparation/RenderObjectPreparation.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

struct MeshRenderItem final
{
	RenderObjectId Object;
	std::uint32_t DrawIndex = 0u;
	MaterialGpuHandle Material;
	RenderMeshInstanceGroupIndex InstanceGroupIndex =
	    kInvalidRenderMeshInstanceGroupIndex;
	RenderMaterialClassification Classification =
	    RenderMaterialClassification::Rejected;
	std::uint32_t RenderStateKey = 0u;
	float CameraDistanceSquared = 0.0f;
};

struct MeshInstanceBatchBuildOptions final
{
	bool EnableAutoBatching = true;
	bool RequireMaterialBindingSet = true;
	bool CollectDiagnostics = false;
};

struct MeshInstanceBatchBuildResult final
{
	std::vector<std::uint32_t> RasterInstanceIndices;
	std::vector<MeshInstanceBatch> Batches;
	MeshGeometryInstancingDiagnostics Diagnostics;
};

class MeshInstanceBatchBuilder final
{
  public:
	MeshInstanceBatchBuilder();
	~MeshInstanceBatchBuilder() noexcept;

	MeshInstanceBatchBuilder(const MeshInstanceBatchBuilder&) = delete;
	MeshInstanceBatchBuilder& operator=(const MeshInstanceBatchBuilder&) = delete;

	void Build(
	    std::span<const MeshRenderItem> renderItems,
	    std::span<const MeshDraw> draws,
	    std::span<const RenderMeshInstanceGroup> instanceGroups,
	    const MeshInstanceBatchBuildOptions& options,
	    MeshInstanceBatchBuildResult& result);

  private:
	struct BuildScratch;
	struct BatchKey final
	{
		GpuMeshHandle Mesh;
		MaterialGpuHandle Material;
		std::uint32_t MaterialSlot = 0u;
		std::uint64_t SkeletonAssetId = 0u;
		RenderMeshKind MeshKind = RenderMeshKind::Static;
		RenderMaterialClassification Classification =
		    RenderMaterialClassification::Rejected;
		std::uint32_t RenderStateKey = 0u;
	};

	static void CollectValidItems(
	    std::span<const MeshRenderItem> renderItems,
	    std::span<const MeshDraw> draws,
	    std::size_t instanceGroupCount,
	    const MeshInstanceBatchBuildOptions& options,
	    BuildScratch& scratch,
	    MeshInstanceBatchBuildResult& result);
	static void CollectPreservedGroupItems(
	    std::span<const MeshRenderItem> renderItems,
	    std::size_t instanceGroupCount,
	    BuildScratch& scratch);
	static void AppendPreservedGroups(
	    std::span<const MeshRenderItem> renderItems,
	    std::span<const MeshDraw> draws,
	    std::span<const RenderMeshInstanceGroup> instanceGroups,
	    const MeshInstanceBatchBuildOptions& options,
	    BuildScratch& scratch,
	    MeshInstanceBatchBuildResult& result);
	static void PartitionRemainingItems(
	    std::span<const MeshRenderItem> renderItems,
	    BuildScratch& scratch);
	static void AppendOpaqueBatches(
	    std::span<const MeshRenderItem> renderItems,
	    std::span<const MeshDraw> draws,
	    const MeshInstanceBatchBuildOptions& options,
	    BuildScratch& scratch,
	    MeshInstanceBatchBuildResult& result);
	static void AppendTransparentBatches(
	    std::span<const MeshRenderItem> renderItems,
	    std::span<const MeshDraw> draws,
	    const MeshInstanceBatchBuildOptions& options,
	    BuildScratch& scratch,
	    MeshInstanceBatchBuildResult& result);
	static void FinalizeDiagnostics(
	    const MeshInstanceBatchBuildOptions& options,
	    const BuildScratch& scratch,
	    MeshInstanceBatchBuildResult& result) noexcept;
	static bool IsValidCandidate(
	    const MeshRenderItem& item,
	    std::span<const MeshDraw> draws,
	    std::size_t instanceGroupCount,
	    const MeshInstanceBatchBuildOptions& options,
	    MeshGeometryInstancingDiagnostics& diagnostics) noexcept;
	static BatchKey MakeBatchKey(
	    const MeshRenderItem& item,
	    std::span<const MeshDraw> draws) noexcept;
	static bool BatchKeyLess(
	    const BatchKey& lhs,
	    const BatchKey& rhs) noexcept;
	static bool CanShareBatch(
	    const MeshRenderItem& lhs,
	    const MeshRenderItem& rhs,
	    std::span<const MeshDraw> draws) noexcept;
	static bool OpaqueItemLess(
	    const MeshRenderItem& lhs,
	    const MeshRenderItem& rhs,
	    std::span<const MeshDraw> draws) noexcept;
	static bool TransparentItemLess(
	    const MeshRenderItem& lhs,
	    const MeshRenderItem& rhs) noexcept;
	static MeshInstanceBatchSource ResolvePreservedGroupSource(
	    RenderMeshInstanceGroupKind groupKind) noexcept;
	static void AppendBatch(
	    std::span<const MeshRenderItem> renderItems,
	    std::span<const MeshDraw> draws,
	    std::span<const std::size_t> itemIndices,
	    MeshInstanceBatchSource source,
	    bool collectDiagnostics,
	    MeshInstanceBatchBuildResult& result);

	std::unique_ptr<BuildScratch> m_scratch;
};
