#include "PCH.h"

#include "SceneData/Builders/MeshInstanceBatchBuilder.h"

#include "Meshes/GPUMesh.h"

MeshInstanceBatchBuildResult MeshInstanceBatchBuilder::Build(
    const std::vector<MeshRenderItem>& renderItems,
    const std::vector<MeshInstanceGroupSnapshot>& instanceGroups,
    const MeshInstanceBatchBuildOptions& options) const
{
	MeshInstanceBatchBuildResult result;
	if (options.collectDiagnostics)
	{
		result.diagnostics.CandidateItemCount = static_cast<std::uint32_t>(renderItems.size());
		result.diagnostics.MeshDrawCount = static_cast<std::uint32_t>(renderItems.size());
	}

	std::vector<std::size_t> validItemIndices;
	validItemIndices.reserve(renderItems.size());
	for (std::size_t itemIndex = 0; itemIndex < renderItems.size(); ++itemIndex)
	{
		if (IsValidCandidate(renderItems[itemIndex], options, result.diagnostics))
		{
			validItemIndices.push_back(itemIndex);
		}
	}

	std::vector<bool> consumedItems(renderItems.size(), false);
	for (std::size_t groupIndex = 0; groupIndex < instanceGroups.size(); ++groupIndex)
	{
		const MeshInstanceGroupSnapshot& group = instanceGroups[groupIndex];
		if (group.groupKind == SceneMeshInstanceGroupKind::None)
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

	if (options.collectDiagnostics)
	{
		result.diagnostics.MeshBatchCount = static_cast<std::uint32_t>(result.batches.size());
		result.diagnostics.SingletonDrawCount = result.diagnostics.MeshDrawCount - result.diagnostics.InstancesInBatches;
	}
	return result;
}

bool MeshInstanceBatchBuilder::IsValidCandidate(
    const MeshRenderItem& item,
    const MeshInstanceBatchBuildOptions& options,
    MeshGeometryInstancingDiagnostics& diagnostics) noexcept
{
	if (item.draw.gpuMesh == nullptr || !item.draw.gpuMesh->IsValid())
	{
		if (options.collectDiagnostics)
		{
			++diagnostics.RejectedCandidateCount;
			++diagnostics.RejectedMissingGpuMeshCount;
		}
		return false;
	}

	if (options.requireMaterialBindingSet && item.materialBindingSet == nullptr)
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
	    .gpuMesh = item.draw.gpuMesh,
	    .materialBindingSet = item.materialBindingSet,
	    .materialSlot = item.draw.materialSlot,
	    .renderStateKey = item.renderStateKey,
	    .renderLayer = item.renderLayer};
}

bool MeshInstanceBatchBuilder::CanShareBatch(const MeshRenderItem& lhs, const MeshRenderItem& rhs) noexcept
{
	return MakeBatchKey(lhs) == MakeBatchKey(rhs);
}

MeshInstanceBatchSource MeshInstanceBatchBuilder::ResolvePreservedGroupSource(SceneMeshInstanceGroupKind groupKind) noexcept
{
	return groupKind == SceneMeshInstanceGroupKind::AuthoredInstanceGroup ? MeshInstanceBatchSource::AuthoredGroup
	                                                                    : MeshInstanceBatchSource::PreservedGroup;
}

void MeshInstanceBatchBuilder::AppendBatch(
    const std::vector<MeshRenderItem>& renderItems,
    const std::vector<std::size_t>& itemIndices,
    MeshInstanceBatchSource source,
	bool collectDiagnostics,
    MeshInstanceBatchBuildResult& result)
{
	if (itemIndices.size() < 2u)
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
	        .gpuMesh = firstItem.draw.gpuMesh,
	        .materialSlot = firstItem.draw.materialSlot,
	        .firstInstance = firstInstance,
	        .instanceCount = static_cast<std::uint32_t>(itemIndices.size()),
	        .source = source});

	if (collectDiagnostics)
	{
		result.diagnostics.InstancesInBatches += static_cast<std::uint32_t>(itemIndices.size());
		result.diagnostics.EstimatedGBufferDrawCallsSaved += static_cast<std::uint32_t>(itemIndices.size() - 1u);
		if (source == MeshInstanceBatchSource::AuthoredGroup)
		{
			++result.diagnostics.AuthoredBatchCount;
		}
		else if (source == MeshInstanceBatchSource::PreservedGroup)
		{
			++result.diagnostics.PreservedGroupBatchCount;
		}
		else
		{
			++result.diagnostics.AutoBatchCount;
		}
	}
}