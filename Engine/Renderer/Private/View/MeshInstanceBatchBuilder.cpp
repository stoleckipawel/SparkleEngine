#include "PCH.h"

#include "View/MeshInstanceBatchBuilder.h"

#include <algorithm>
#include <array>

struct MeshInstanceBatchBuilder::BuildScratch final
{
	std::vector<std::size_t> ValidItemIndices;
	std::vector<bool> ConsumedItems;
	std::vector<std::vector<std::size_t>> GroupItems;
	std::vector<std::size_t> OpaqueItems;
	std::vector<std::size_t> TransparentItems;
};

MeshInstanceBatchBuilder::MeshInstanceBatchBuilder() :
    m_scratch(std::make_unique<BuildScratch>())
{
}

MeshInstanceBatchBuilder::~MeshInstanceBatchBuilder() noexcept = default;

void MeshInstanceBatchBuilder::Build(
    std::span<const MeshRenderItem> renderItems,
    std::span<const PreparedRenderPrimitive> primitives,
    std::span<const RenderMeshInstanceGroup> instanceGroups,
    const MeshInstanceBatchBuildOptions& options,
    MeshInstanceBatchBuildResult& result)
{
	BuildScratch& scratch = *m_scratch;

	result.RasterInstanceIndices.clear();
	result.Batches.clear();
	result.Diagnostics = {};

	scratch.ValidItemIndices.clear();
	scratch.ConsumedItems.clear();
	for (std::vector<std::size_t>& groupItems : scratch.GroupItems)
	{
		groupItems.clear();
	}
	scratch.OpaqueItems.clear();
	scratch.TransparentItems.clear();

	CollectValidItems(renderItems, primitives, instanceGroups.size(), options, scratch, result);
	CollectPreservedGroupItems(renderItems, instanceGroups.size(), scratch);
	AppendPreservedGroups(renderItems, primitives, instanceGroups, options, scratch, result);
	PartitionRemainingItems(renderItems, scratch);
	AppendOpaqueBatches(renderItems, primitives, options, scratch, result);
	AppendTransparentBatches(renderItems, primitives, options, scratch, result);
	FinalizeDiagnostics(options, scratch, result);
}

void MeshInstanceBatchBuilder::CollectValidItems(
    std::span<const MeshRenderItem> renderItems,
    std::span<const PreparedRenderPrimitive> primitives,
    std::size_t instanceGroupCount,
    const MeshInstanceBatchBuildOptions& options,
    BuildScratch& scratch,
    MeshInstanceBatchBuildResult& result)
{
	if (options.CollectDiagnostics)
	{
		result.Diagnostics.CandidateItemCount = static_cast<std::uint32_t>(renderItems.size());
	}

	scratch.ValidItemIndices.reserve(renderItems.size());
	for (std::size_t itemIndex = 0u; itemIndex < renderItems.size(); ++itemIndex)
	{
		if (IsValidCandidate(renderItems[itemIndex], primitives, instanceGroupCount, options, result.Diagnostics))
		{
			scratch.ValidItemIndices.push_back(itemIndex);
		}
	}
}

void MeshInstanceBatchBuilder::CollectPreservedGroupItems(
    std::span<const MeshRenderItem> renderItems,
    std::size_t instanceGroupCount,
    BuildScratch& scratch)
{
	scratch.ConsumedItems.assign(renderItems.size(), false);
	scratch.GroupItems.resize(instanceGroupCount);

	for (const std::size_t itemIndex : scratch.ValidItemIndices)
	{
		const MeshRenderItem& item = renderItems[itemIndex];
		const bool hasGroup =
		    item.InstanceGroupIndex != kInvalidRenderMeshInstanceGroupIndex && item.InstanceGroupIndex < scratch.GroupItems.size();
		if (hasGroup && item.Classification != RenderMaterialClassification::Transparent)
		{
			scratch.GroupItems[item.InstanceGroupIndex].push_back(itemIndex);
		}
	}
}

void MeshInstanceBatchBuilder::AppendPreservedGroups(
    std::span<const MeshRenderItem> renderItems,
    std::span<const PreparedRenderPrimitive> primitives,
    std::span<const RenderMeshInstanceGroup> instanceGroups,
    const MeshInstanceBatchBuildOptions& options,
    BuildScratch& scratch,
    MeshInstanceBatchBuildResult& result)
{
	for (std::size_t groupIndex = 0u; groupIndex < instanceGroups.size(); ++groupIndex)
	{
		const RenderMeshInstanceGroup& group = instanceGroups[groupIndex];
		std::vector<std::size_t>& items = scratch.GroupItems[groupIndex];
		if (group.groupKind == RenderMeshInstanceGroupKind::None || items.size() < 2u)
		{
			continue;
		}

		const bool compatible = std::all_of(
		    items.begin() + 1u,
		    items.end(),
		    [&renderItems, &primitives, &items](std::size_t itemIndex)
		    { return CanShareBatch(renderItems[items.front()], renderItems[itemIndex], primitives); });
		if (!compatible)
		{
			if (options.CollectDiagnostics)
			{
				++result.Diagnostics.RejectedIncompatibleGroupCount;
			}
			continue;
		}

		AppendBatch(renderItems, primitives, items, ResolvePreservedGroupSource(group.groupKind), options.CollectDiagnostics, result);
		for (const std::size_t itemIndex : items)
		{
			scratch.ConsumedItems[itemIndex] = true;
		}
	}
}

void MeshInstanceBatchBuilder::PartitionRemainingItems(std::span<const MeshRenderItem> renderItems, BuildScratch& scratch)
{
	scratch.OpaqueItems.reserve(scratch.ValidItemIndices.size());
	scratch.TransparentItems.reserve(scratch.ValidItemIndices.size());

	for (const std::size_t itemIndex : scratch.ValidItemIndices)
	{
		if (scratch.ConsumedItems[itemIndex])
		{
			continue;
		}

		const MeshRenderItem& item = renderItems[itemIndex];
		std::vector<std::size_t>& destination =
		    item.Classification == RenderMaterialClassification::Transparent ? scratch.TransparentItems : scratch.OpaqueItems;
		destination.push_back(itemIndex);
	}
}

void MeshInstanceBatchBuilder::AppendOpaqueBatches(
    std::span<const MeshRenderItem> renderItems,
    std::span<const PreparedRenderPrimitive> primitives,
    const MeshInstanceBatchBuildOptions& options,
    BuildScratch& scratch,
    MeshInstanceBatchBuildResult& result)
{
	std::stable_sort(
	    scratch.OpaqueItems.begin(),
	    scratch.OpaqueItems.end(),
	    [&renderItems, &primitives](std::size_t lhs, std::size_t rhs)
	    { return OpaqueItemLess(renderItems[lhs], renderItems[rhs], primitives); });

	for (std::size_t begin = 0u; begin < scratch.OpaqueItems.size();)
	{
		std::size_t end = begin + 1u;
		if (options.EnableAutoBatching)
		{
			while (end < scratch.OpaqueItems.size()
			    && CanShareBatch(renderItems[scratch.OpaqueItems[begin]], renderItems[scratch.OpaqueItems[end]], primitives))
			{
				++end;
			}
		}

		const std::span<const std::size_t> batchItems{scratch.OpaqueItems.data() + begin, end - begin};
		AppendBatch(
		    renderItems,
		    primitives,
		    batchItems,
		    batchItems.size() > 1u ? MeshInstanceBatchSource::AutoBatch : MeshInstanceBatchSource::SingleInstance,
		    options.CollectDiagnostics,
		    result);
		begin = end;
	}
}

void MeshInstanceBatchBuilder::AppendTransparentBatches(
    std::span<const MeshRenderItem> renderItems,
    std::span<const PreparedRenderPrimitive> primitives,
    const MeshInstanceBatchBuildOptions& options,
    BuildScratch& scratch,
    MeshInstanceBatchBuildResult& result)
{
	std::stable_sort(
	    scratch.TransparentItems.begin(),
	    scratch.TransparentItems.end(),
	    [&renderItems](std::size_t lhs, std::size_t rhs) { return TransparentItemLess(renderItems[lhs], renderItems[rhs]); });

	for (const std::size_t itemIndex : scratch.TransparentItems)
	{
		const std::array<std::size_t, 1> single{itemIndex};
		AppendBatch(renderItems, primitives, single, MeshInstanceBatchSource::SingleInstance, options.CollectDiagnostics, result);
	}
}

void MeshInstanceBatchBuilder::FinalizeDiagnostics(
    const MeshInstanceBatchBuildOptions& options,
    const BuildScratch& scratch,
    MeshInstanceBatchBuildResult& result) noexcept
{
	if (!options.CollectDiagnostics)
	{
		return;
	}

	result.Diagnostics.RenderableInstanceCount = static_cast<std::uint32_t>(scratch.ValidItemIndices.size());
	result.Diagnostics.MeshBatchCount = static_cast<std::uint32_t>(result.Batches.size());
}

bool MeshInstanceBatchBuilder::IsValidCandidate(
    const MeshRenderItem& item,
    std::span<const PreparedRenderPrimitive> primitives,
    std::size_t instanceGroupCount,
    const MeshInstanceBatchBuildOptions& options,
    MeshGeometryInstancingDiagnostics& diagnostics) noexcept
{
	if (item.DrawIndex >= primitives.size() || !primitives[item.DrawIndex].Draw.Geometry.Mesh)
	{
		if (options.CollectDiagnostics)
		{
			++diagnostics.RejectedCandidateCount;
			++diagnostics.RejectedMissingGpuMeshCount;
		}
		return false;
	}
	if (item.InstanceGroupIndex != kInvalidRenderMeshInstanceGroupIndex && item.InstanceGroupIndex >= instanceGroupCount)
	{
		if (options.CollectDiagnostics)
		{
			++diagnostics.RejectedCandidateCount;
			++diagnostics.RejectedInvalidInstanceGroupCount;
		}
		return false;
	}
	if (options.RequireMaterialBindingSet && !item.Material)
	{
		if (options.CollectDiagnostics)
		{
			++diagnostics.RejectedCandidateCount;
			++diagnostics.RejectedInvalidMaterialCount;
		}
		return false;
	}
	return item.Classification != RenderMaterialClassification::Rejected;
}

MeshInstanceBatchBuilder::BatchKey MeshInstanceBatchBuilder::MakeBatchKey(
    const MeshRenderItem& item,
    std::span<const PreparedRenderPrimitive> primitives) noexcept
{
	const MeshDraw& draw = primitives[item.DrawIndex].Draw;
	return BatchKey{
	    .Mesh = draw.Geometry.Mesh,
	    .Material = item.Material,
	    .MaterialSlot = draw.MaterialSlot,
	    .SkeletonAssetId = draw.Skinning.SkeletonAssetId,
	    .MeshKind = draw.Geometry.MeshKind,
	    .Classification = item.Classification,
	    .RenderStateKey = item.RenderStateKey};
}

bool MeshInstanceBatchBuilder::BatchKeyLess(const BatchKey& lhs, const BatchKey& rhs) noexcept
{
	if (lhs.Classification != rhs.Classification)
	{
		return lhs.Classification < rhs.Classification;
	}
	if (lhs.RenderStateKey != rhs.RenderStateKey)
	{
		return lhs.RenderStateKey < rhs.RenderStateKey;
	}
	if (lhs.Material.Generation != rhs.Material.Generation)
	{
		return lhs.Material.Generation < rhs.Material.Generation;
	}
	if (lhs.Material.Index != rhs.Material.Index)
	{
		return lhs.Material.Index < rhs.Material.Index;
	}
	if (lhs.Mesh.Value != rhs.Mesh.Value)
	{
		return lhs.Mesh.Value < rhs.Mesh.Value;
	}
	if (lhs.MaterialSlot != rhs.MaterialSlot)
	{
		return lhs.MaterialSlot < rhs.MaterialSlot;
	}
	if (lhs.SkeletonAssetId != rhs.SkeletonAssetId)
	{
		return lhs.SkeletonAssetId < rhs.SkeletonAssetId;
	}
	return lhs.MeshKind < rhs.MeshKind;
}

bool MeshInstanceBatchBuilder::CanShareBatch(
    const MeshRenderItem& lhs,
    const MeshRenderItem& rhs,
    std::span<const PreparedRenderPrimitive> primitives) noexcept
{
	if (lhs.Classification == RenderMaterialClassification::Transparent || rhs.Classification == RenderMaterialClassification::Transparent)
	{
		return false;
	}
	const BatchKey left = MakeBatchKey(lhs, primitives);
	const BatchKey right = MakeBatchKey(rhs, primitives);
	return !BatchKeyLess(left, right) && !BatchKeyLess(right, left);
}

bool MeshInstanceBatchBuilder::OpaqueItemLess(
    const MeshRenderItem& lhs,
    const MeshRenderItem& rhs,
    std::span<const PreparedRenderPrimitive> primitives) noexcept
{
	const BatchKey left = MakeBatchKey(lhs, primitives);
	const BatchKey right = MakeBatchKey(rhs, primitives);
	if (BatchKeyLess(left, right))
	{
		return true;
	}
	if (BatchKeyLess(right, left))
	{
		return false;
	}
	return lhs.Object < rhs.Object;
}

bool MeshInstanceBatchBuilder::TransparentItemLess(const MeshRenderItem& lhs, const MeshRenderItem& rhs) noexcept
{
	if (lhs.CameraDistanceSquared != rhs.CameraDistanceSquared)
	{
		return lhs.CameraDistanceSquared > rhs.CameraDistanceSquared;
	}
	return lhs.Object < rhs.Object;
}

MeshInstanceBatchSource MeshInstanceBatchBuilder::ResolvePreservedGroupSource(RenderMeshInstanceGroupKind groupKind) noexcept
{
	return groupKind == RenderMeshInstanceGroupKind::AuthoredInstanceGroup ? MeshInstanceBatchSource::AuthoredGroup
	                                                                       : MeshInstanceBatchSource::PreservedGroup;
}

void MeshInstanceBatchBuilder::AppendBatch(
    std::span<const MeshRenderItem> renderItems,
    std::span<const PreparedRenderPrimitive> primitives,
    std::span<const std::size_t> itemIndices,
    MeshInstanceBatchSource source,
    bool collectDiagnostics,
    MeshInstanceBatchBuildResult& result)
{
	if (itemIndices.empty())
	{
		return;
	}

	const std::uint32_t firstInstance = static_cast<std::uint32_t>(result.RasterInstanceIndices.size());
	for (const std::size_t itemIndex : itemIndices)
	{
		result.RasterInstanceIndices.push_back(renderItems[itemIndex].DrawIndex);
	}

	const MeshDraw& firstDraw = primitives[renderItems[itemIndices.front()].DrawIndex].Draw;
	result.Batches.push_back(
	    MeshInstanceBatch{
	        .Mesh = firstDraw.Geometry.Mesh,
	        .materialSlot = firstDraw.MaterialSlot,
	        .firstInstance = firstInstance,
	        .instanceCount = static_cast<std::uint32_t>(itemIndices.size()),
	        .meshKind = firstDraw.Geometry.MeshKind,
	        .materialClassification = renderItems[itemIndices.front()].Classification,
	        .source = source});

	if (!collectDiagnostics)
	{
		return;
	}

	const std::uint32_t instanceCount = static_cast<std::uint32_t>(itemIndices.size());
	result.Diagnostics.SubmittedInstanceCount += instanceCount;
	result.Diagnostics.EstimatedGBufferDrawCallsSaved += instanceCount - 1u;
	result.Diagnostics.MinInstancesPerBatch =
	    result.Diagnostics.MinInstancesPerBatch == 0u ? instanceCount : (std::min) (result.Diagnostics.MinInstancesPerBatch, instanceCount);
	result.Diagnostics.MaxInstancesPerBatch = (std::max) (result.Diagnostics.MaxInstancesPerBatch, instanceCount);
	switch (source)
	{
		case MeshInstanceBatchSource::AuthoredGroup:
			++result.Diagnostics.AuthoredBatchCount;
			break;
		case MeshInstanceBatchSource::PreservedGroup:
			++result.Diagnostics.PreservedGroupBatchCount;
			break;
		case MeshInstanceBatchSource::AutoBatch:
			++result.Diagnostics.AutoBatchCount;
			break;
		case MeshInstanceBatchSource::SingleInstance:
			++result.Diagnostics.SingleInstanceBatchCount;
			break;
	}
}
