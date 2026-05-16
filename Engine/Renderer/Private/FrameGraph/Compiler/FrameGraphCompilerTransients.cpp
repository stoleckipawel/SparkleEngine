#include "PCH.h"
#include "FrameGraphCompiler.h"

#include <algorithm>
#include <cassert>

namespace
{
	using AllocationPool = FrameGraphTransientResourcePlan::AllocationPool;

	bool AreTextureResourceDescsEqual(const RhiTextureResourceDesc& lhs, const RhiTextureResourceDesc& rhs) noexcept
	{
		return lhs.Width == rhs.Width && lhs.Height == rhs.Height && lhs.Format == rhs.Format && lhs.MipLevels == rhs.MipLevels &&
		       lhs.AllowRenderTarget == rhs.AllowRenderTarget && lhs.AllowDepthStencil == rhs.AllowDepthStencil &&
		       lhs.AllowUnorderedAccess == rhs.AllowUnorderedAccess;
	}

	bool AreBufferResourceDescsEqual(const RhiBufferResourceDesc& lhs, const RhiBufferResourceDesc& rhs) noexcept
	{
		return lhs.SizeInBytes == rhs.SizeInBytes && lhs.StrideInBytes == rhs.StrideInBytes &&
		       lhs.AllowUnorderedAccess == rhs.AllowUnorderedAccess;
	}

	bool AreClearValuesEqual(const RhiOptimizedClearValue& lhs, const RhiOptimizedClearValue& rhs, FrameGraphResourceKind kind) noexcept
	{
		if (lhs.ValueType != rhs.ValueType || lhs.Format != rhs.Format)
		{
			return false;
		}

		if (kind == FrameGraphResourceKind::DepthStencil)
		{
			return lhs.Depth == rhs.Depth && lhs.Stencil == rhs.Stencil;
		}

		if (kind == FrameGraphResourceKind::Buffer)
		{
			return true;
		}

		for (std::size_t colorIndex = 0; colorIndex < 4; ++colorIndex)
		{
			if (lhs.Color[colorIndex] != rhs.Color[colorIndex])
			{
				return false;
			}
		}

		return true;
	}

	bool CanSharePhysicalBlock(
	    const FrameGraphPhysicalAllocationPlan& block,
	    const FrameGraphTransientResourcePlan& transientPlan) noexcept
	{
		const auto& physicalPlan = transientPlan.physicalAllocation;
		if (block.pool != physicalPlan.pool)
		{
			return false;
		}

		if (block.lastExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX ||
		    transientPlan.firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX)
		{
			return false;
		}

		if (block.lastExecutionIndex >= transientPlan.firstExecutionIndex)
		{
			return false;
		}

		if (block.alignment != physicalPlan.alignment || block.sizeInBytes < physicalPlan.sizeInBytes ||
		    block.heapOffset != physicalPlan.heapOffset)
		{
			return false;
		}

		if (block.pool == AllocationPool::Buffer)
		{
			if (!AreBufferResourceDescsEqual(block.bufferResourceDesc, physicalPlan.bufferResourceDesc))
			{
				return false;
			}
		}
		else if (!AreTextureResourceDescsEqual(block.textureResourceDesc, physicalPlan.textureResourceDesc))
		{
			return false;
		}

		if (block.hasOptimizedClearValue != physicalPlan.hasOptimizedClearValue)
		{
			return false;
		}

		if (block.hasOptimizedClearValue &&
		    !AreClearValuesEqual(block.optimizedClearValue, physicalPlan.optimizedClearValue, transientPlan.kind))
		{
			return false;
		}

		return true;
	}
}  // namespace

void FrameGraphCompiler::BuildTransientResourceLifetimes() noexcept
{
	for (FrameGraphTransientResourcePlan& transientPlan : m_plan.transientResources)
	{
		transientPlan.firstUserPass = INVALID_FRAME_GRAPH_PASS_INDEX;
		transientPlan.lastUserPass = INVALID_FRAME_GRAPH_PASS_INDEX;
		transientPlan.firstExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
		transientPlan.lastExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
		transientPlan.readUsed = false;
		transientPlan.writeUsed = false;
		transientPlan.requiredStates.clear();
	}

	for (std::size_t executionIndex = 0; executionIndex < m_plan.executionOrder.size(); ++executionIndex)
	{
		const FrameGraphPassIndex passIndex = m_plan.executionOrder[executionIndex];
		const FrameGraphPassNode& passRecord = m_plan.passes[passIndex];

		for (const PassResourceDeclaration& declaration : passRecord.declarations)
		{
			if (!declaration.handle.IsValid())
			{
				continue;
			}

			FrameGraphTransientResourcePlan* transientPlan = FindTransientResourcePlan(declaration.handle);
			if (transientPlan == nullptr)
			{
				continue;
			}

			const FrameGraphResourceNode& compiledResource = GetCompiledResourceEntry(declaration.handle);
			assert(compiledResource.ownership == FrameGraphResourceOwnership::Transient);

			const FrameGraphPassIndex executionIndexValue = static_cast<FrameGraphPassIndex>(executionIndex);
			if (transientPlan->firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX)
			{
				transientPlan->firstExecutionIndex = executionIndexValue;
				transientPlan->firstUserPass = passIndex;
			}

			transientPlan->lastExecutionIndex = executionIndexValue;
			transientPlan->lastUserPass = passIndex;

			if (IsReadOnlyUsage(declaration.usage))
			{
				transientPlan->readUsed = true;
			}
			else if (IsWriteOnlyUsage(declaration.usage))
			{
				transientPlan->writeUsed = true;
			}

			const ResourceState requiredState = InferRequiredResourceState(declaration, compiledResource);
			const auto stateIt = std::find(transientPlan->requiredStates.begin(), transientPlan->requiredStates.end(), requiredState);
			if (stateIt == transientPlan->requiredStates.end())
			{
				transientPlan->requiredStates.push_back(requiredState);
			}
		}
	}

	for (const FrameGraphTransientResourcePlan& transientPlan : m_plan.transientResources)
	{
		assert(transientPlan.handle.IsValid());
		assert(transientPlan.firstExecutionIndex != INVALID_FRAME_GRAPH_PASS_INDEX);
		assert(transientPlan.lastExecutionIndex != INVALID_FRAME_GRAPH_PASS_INDEX);
		assert(transientPlan.firstExecutionIndex <= transientPlan.lastExecutionIndex);
		assert(transientPlan.firstUserPass != INVALID_FRAME_GRAPH_PASS_INDEX);
		assert(transientPlan.lastUserPass != INVALID_FRAME_GRAPH_PASS_INDEX);
		assert(!transientPlan.requiredStates.empty());
	}
}

void FrameGraphCompiler::BuildTransientPhysicalBlockAssignments() noexcept
{
	m_plan.physicalBlocks.clear();

	std::vector<FrameGraphTransientResourcePlan*> assignmentOrder;
	assignmentOrder.reserve(m_plan.transientResources.size());
	for (FrameGraphTransientResourcePlan& transientPlan : m_plan.transientResources)
	{
		transientPlan.physicalAllocation.physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
		assignmentOrder.push_back(&transientPlan);
	}

	std::sort(
	    assignmentOrder.begin(),
	    assignmentOrder.end(),
	    [](const FrameGraphTransientResourcePlan* lhs, const FrameGraphTransientResourcePlan* rhs)
	    {
		    if (lhs->physicalAllocation.pool != rhs->physicalAllocation.pool)
		    {
			    return lhs->physicalAllocation.pool < rhs->physicalAllocation.pool;
		    }

		    if (lhs->firstExecutionIndex != rhs->firstExecutionIndex)
		    {
			    return lhs->firstExecutionIndex < rhs->firstExecutionIndex;
		    }

		    return lhs->handle.index < rhs->handle.index;
	    });

	for (FrameGraphTransientResourcePlan* transientPlan : assignmentOrder)
	{
		assert(transientPlan != nullptr);

		FrameGraphPhysicalAllocationPlan* selectedBlock = nullptr;
		for (FrameGraphPhysicalAllocationPlan& block : m_plan.physicalBlocks)
		{
			if (!CanSharePhysicalBlock(block, *transientPlan))
			{
				continue;
			}

			selectedBlock = &block;
			break;
		}

		if (selectedBlock == nullptr)
		{
			const std::uint32_t blockIndex = static_cast<std::uint32_t>(m_plan.physicalBlocks.size());
			m_plan.physicalBlocks.push_back(
			    FrameGraphPhysicalAllocationPlan{
			        .physicalBlockIndex = blockIndex,
			        .pool = transientPlan->physicalAllocation.pool,
			        .sizeInBytes = transientPlan->physicalAllocation.sizeInBytes,
			        .alignment = transientPlan->physicalAllocation.alignment,
			        .heapOffset = transientPlan->physicalAllocation.heapOffset,
			        .textureResourceDesc = transientPlan->physicalAllocation.textureResourceDesc,
			        .bufferResourceDesc = transientPlan->physicalAllocation.bufferResourceDesc,
			        .optimizedClearValue = transientPlan->physicalAllocation.optimizedClearValue,
			        .hasOptimizedClearValue = transientPlan->physicalAllocation.hasOptimizedClearValue,
			        .firstExecutionIndex = transientPlan->firstExecutionIndex,
			        .lastExecutionIndex = transientPlan->lastExecutionIndex,
			        .handles = {transientPlan->handle}});
			selectedBlock = &m_plan.physicalBlocks.back();
		}
		else
		{
			selectedBlock->lastExecutionIndex = transientPlan->lastExecutionIndex;
			selectedBlock->handles.push_back(transientPlan->handle);
		}

		transientPlan->physicalAllocation.physicalBlockIndex = selectedBlock->physicalBlockIndex;
	}

	for (const FrameGraphTransientResourcePlan& transientPlan : m_plan.transientResources)
	{
		assert(transientPlan.physicalAllocation.physicalBlockIndex != INVALID_FRAME_GRAPH_RESOURCE_INDEX);
	}
}

void FrameGraphCompiler::BuildTransientAliasingBarriers() noexcept
{
	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		passRecord.compiledAliasingBarriers.clear();
	}
	m_plan.finalAliasingBarriers.clear();

	for (const FrameGraphPhysicalAllocationPlan& block : m_plan.physicalBlocks)
	{
		if (block.handles.size() < 2)
		{
			continue;
		}

		std::vector<const FrameGraphTransientResourcePlan*> orderedPlans;
		orderedPlans.reserve(block.handles.size());
		for (const FrameGraphResourceHandle handle : block.handles)
		{
			const FrameGraphTransientResourcePlan* transientPlan = FindTransientResourcePlan(handle);
			assert(transientPlan != nullptr);
			orderedPlans.push_back(transientPlan);
		}

		std::sort(
		    orderedPlans.begin(),
		    orderedPlans.end(),
		    [](const FrameGraphTransientResourcePlan* lhs, const FrameGraphTransientResourcePlan* rhs)
		    {
			    if (lhs->firstExecutionIndex != rhs->firstExecutionIndex)
			    {
				    return lhs->firstExecutionIndex < rhs->firstExecutionIndex;
			    }

			    return lhs->handle.index < rhs->handle.index;
		    });

		for (std::size_t ownerIndex = 1; ownerIndex < orderedPlans.size(); ++ownerIndex)
		{
			const FrameGraphTransientResourcePlan& previousOwner = *orderedPlans[ownerIndex - 1];
			const FrameGraphTransientResourcePlan& nextOwner = *orderedPlans[ownerIndex];

			assert(previousOwner.lastExecutionIndex < nextOwner.firstExecutionIndex);

			FrameGraphAliasingBarrier barrier{
			    .physicalBlockIndex = block.physicalBlockIndex,
			    .beforeHandle = previousOwner.handle,
			    .afterHandle = nextOwner.handle,
			    .executeBeforePass = previousOwner.lastUserPass,
			    .executeAfterPass = nextOwner.firstUserPass};

			assert(barrier.executeAfterPass != INVALID_FRAME_GRAPH_PASS_INDEX);
			assert(barrier.executeAfterPass < m_plan.passes.size());
			m_plan.passes[barrier.executeAfterPass].compiledAliasingBarriers.push_back(barrier);
		}
	}
}
