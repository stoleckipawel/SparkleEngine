#include "PCH.h"

#include "SceneData/Builders/MeshInstanceBatchBuilder.h"

#include "Meshes/GPUMesh.h"

#include <algorithm>

MeshInstanceBatchBuildResult MeshInstanceBatchBuilder::Build(
    const std::vector<MeshRenderItem>& renderItems,
    const std::vector<RenderMeshInstanceGroup>& instanceGroups,
    const MeshInstanceBatchBuildOptions& options) const
{
	MeshInstanceBatchBuildResult result;
	if (options.collectDiagnostics)
	{
		result.diagnostics.CandidateItemCount = static_cast<std::uint32_t>(renderItems.size());
	}

	std::vector<std::size_t> validItemIndices;
	validItemIndices.reserve(renderItems.size());
	for (std::size_t itemIndex = 0; itemIndex < renderItems.size(); ++itemIndex)
	{
		if (IsValidCandidate(renderItems[itemIndex], instanceGroups.size(), options, result.diagnostics))
		{
			validItemIndices.push_back(itemIndex);
		}
	}
	if (options.collectDiagnostics)
	{
		result.diagnostics.RenderableInstanceCount = static_cast<std::uint32_t>(validItemIndices.size());
	}

	std::vector<bool> consumedItems(renderItems.size(), false);
	for (std::size_t groupIndex = 0; groupIndex < instanceGroups.size(); ++groupIndex)
	{
		const RenderMeshInstanceGroup& group = instanceGroups[groupIndex];
		if (group.groupKind == RenderMeshInstanceGroupKind::None)
		{
			continue;
		}

		std::vector<std::size_t> groupItemIndices;
		groupItemIndices.reserve(group.instanceCount);
		for (const std::size_t itemIndex : validItemIndices)
		{
			if (renderItems[itemIndex].instanceGroupIndex == groupIndex)
			{
				groupItemIndices.push_back(itemIndex);
			}
		}

		if (groupItemIndices.size() < 2u)
		{
			continue;
		}

		bool compatible = true;
		for (std::size_t itemOffset = 1; itemOffset < groupItemIndices.size(); ++itemOffset)
		{
			if (!CanShareBatch(renderItems[groupItemIndices.front()], renderItems[groupItemIndices[itemOffset]]))
			{
				compatible = false;
				break;
			}
		}

		if (!compatible)
		{
			if (options.collectDiagnostics)
			{
				++result.diagnostics.RejectedIncompatibleGroupCount;
			}
			continue;
		}

		AppendBatch(renderItems, groupItemIndices, ResolvePreservedGroupSource(group.groupKind), options.collectDiagnostics, result);
		for (const std::size_t itemIndex : groupItemIndices)
		{
			consumedItems[itemIndex] = true;
		}
	}

	if (options.enableAutoBatching)
	{
		std::vector<std::size_t> autoBatchCandidates;
		autoBatchCandidates.reserve(validItemIndices.size());
		for (const std::size_t itemIndex : validItemIndices)
		{
			if (!consumedItems[itemIndex])
			{
				autoBatchCandidates.push_back(itemIndex);
			}
		}

		for (std::size_t candidateOffset = 0; candidateOffset < autoBatchCandidates.size(); ++candidateOffset)
		{
			const std::size_t seedItemIndex = autoBatchCandidates[candidateOffset];
			if (consumedItems[seedItemIndex])
			{
				continue;
			}

			std::vector<std::size_t> batchItemIndices;
			batchItemIndices.push_back(seedItemIndex);
			for (std::size_t scanOffset = candidateOffset + 1u; scanOffset < autoBatchCandidates.size(); ++scanOffset)
			{
				const std::size_t itemIndex = autoBatchCandidates[scanOffset];
				if (!consumedItems[itemIndex] && CanShareBatch(renderItems[seedItemIndex], renderItems[itemIndex]))
				{
					batchItemIndices.push_back(itemIndex);
				}
			}

			if (batchItemIndices.size() > 1u)
			{
				AppendBatch(renderItems, batchItemIndices, MeshInstanceBatchSource::AutoBatch, options.collectDiagnostics, result);
				for (const std::size_t itemIndex : batchItemIndices)
				{
					consumedItems[itemIndex] = true;
				}
			}
		}
	}

	for (const std::size_t itemIndex : validItemIndices)
	{
		if (!consumedItems[itemIndex])
		{
			AppendBatch(renderItems, std::vector<std::size_t>{itemIndex}, MeshInstanceBatchSource::SingleInstance, options.collectDiagnostics, result);
			consumedItems[itemIndex] = true;
		}
	}

	if (options.collectDiagnostics)
	{
		result.diagnostics.MeshBatchCount = static_cast<std::uint32_t>(result.batches.size());
	}
	return result;
}

bool MeshInstanceBatchBuilder::IsValidCandidate(
    const MeshRenderItem& item,
	std::size_t instanceGroupCount,
    const MeshInstanceBatchBuildOptions& options,
    MeshGeometryInstancingDiagnostics& diagnostics) noexcept
{
	if (!item.draw.Geometry.Mesh)
	{
		if (options.collectDiagnostics)
		{
			++diagnostics.RejectedCandidateCount;
			++diagnostics.RejectedMissingGpuMeshCount;
		}
		return false;
	}

	if (item.instanceGroupIndex != kInvalidRenderMeshInstanceGroupIndex && item.instanceGroupIndex >= instanceGroupCount)
	{
		if (options.collectDiagnostics)
		{
			++diagnostics.RejectedCandidateCount;
			++diagnostics.RejectedInvalidInstanceGroupCount;
		}
		return false;
	}

	if (options.requireMaterialBindingSet && !item.materialGpuHandle)
	{
		if (options.collectDiagnostics)
		{
			++diagnostics.RejectedCandidateCount;
			++diagnostics.RejectedInvalidMaterialCount;
		}
		return false;
	}

	return true;
}

MeshInstanceBatchBuilder::BatchKey MeshInstanceBatchBuilder::MakeBatchKey(const MeshRenderItem& item) noexcept
{
	return BatchKey{
	    .Mesh = item.draw.Geometry.Mesh,
	    .materialGpuHandle = item.materialGpuHandle,
	    .materialSlot = item.draw.Material.Slot,
	    .skeletonAssetId = item.draw.Skinning.SkeletonAssetId,
	    .meshKind = item.draw.Geometry.MeshKind,
	    .renderStateKey = item.renderStateKey,
	    .renderLayer = item.renderLayer};
}

bool MeshInstanceBatchBuilder::CanShareBatch(const MeshRenderItem& lhs, const MeshRenderItem& rhs) noexcept
{
	return MakeBatchKey(lhs) == MakeBatchKey(rhs);
}

MeshInstanceBatchSource MeshInstanceBatchBuilder::ResolvePreservedGroupSource(RenderMeshInstanceGroupKind groupKind) noexcept
{
	return groupKind == RenderMeshInstanceGroupKind::AuthoredInstanceGroup ? MeshInstanceBatchSource::AuthoredGroup
	                                                                      : MeshInstanceBatchSource::PreservedGroup;
}

void MeshInstanceBatchBuilder::AppendBatch(
    const std::vector<MeshRenderItem>& renderItems,
    const std::vector<std::size_t>& itemIndices,
    MeshInstanceBatchSource source,
	bool collectDiagnostics,
    MeshInstanceBatchBuildResult& result)
{
	if (itemIndices.empty())
	{
		return;
	}

	const std::uint32_t firstInstance = static_cast<std::uint32_t>(result.batchInstances.size());
	for (const std::size_t itemIndex : itemIndices)
	{
		result.batchInstances.push_back(renderItems[itemIndex].draw);
	}

	const MeshRenderItem& firstItem = renderItems[itemIndices.front()];
	result.batches.push_back(
	    MeshInstanceBatch{
	        .Mesh = firstItem.draw.Geometry.Mesh,
	        .materialSlot = firstItem.draw.Material.Slot,
	        .firstInstance = firstInstance,
	        .instanceCount = static_cast<std::uint32_t>(itemIndices.size()),
	        .meshKind = firstItem.draw.Geometry.MeshKind,
	        .source = source});

	if (collectDiagnostics)
	{
		const std::uint32_t instanceCount = static_cast<std::uint32_t>(itemIndices.size());
		result.diagnostics.SubmittedInstanceCount += static_cast<std::uint32_t>(itemIndices.size());
		result.diagnostics.EstimatedGBufferDrawCallsSaved += static_cast<std::uint32_t>(itemIndices.size() - 1u);
		result.diagnostics.MinInstancesPerBatch = result.diagnostics.MinInstancesPerBatch == 0
		                                           ? instanceCount
		                                           : (std::min) (result.diagnostics.MinInstancesPerBatch, instanceCount);
		result.diagnostics.MaxInstancesPerBatch = (std::max) (result.diagnostics.MaxInstancesPerBatch, instanceCount);
		if (source == MeshInstanceBatchSource::AuthoredGroup)
		{
			++result.diagnostics.AuthoredBatchCount;
		}
		else if (source == MeshInstanceBatchSource::PreservedGroup)
		{
			++result.diagnostics.PreservedGroupBatchCount;
		}
		else if (source == MeshInstanceBatchSource::AutoBatch)
		{
			++result.diagnostics.AutoBatchCount;
		}
		else
		{
			++result.diagnostics.SingleInstanceBatchCount;
		}
	}
}
