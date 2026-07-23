#include "PCH.h"
#include "FrameGraphCompiler.h"

#include <algorithm>
#include <cassert>

class FrameGraphCompilerTransientsOperations final
{
  public:
	static bool AreClearValuesEqual(const RhiOptimizedClearValue& lhs, const RhiOptimizedClearValue& rhs, FrameGraphResourceKind kind) noexcept
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

	static bool CanSharePhysicalBlock(
	    const FrameGraphTransientResourcePlan& currentOwner,
	    const FrameGraphTransientResourcePlan& transientPlan) noexcept
	{
		const auto& ownerPhysicalPlan = currentOwner.physicalAllocation;
		const auto& physicalPlan = transientPlan.physicalAllocation;
		if (ownerPhysicalPlan.pool != physicalPlan.pool)
		{
			return false;
		}

		if (currentOwner.lifetime.lastExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX ||
		    transientPlan.lifetime.firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX)
		{
			return false;
		}

		if (currentOwner.lifetime.lastExecutionIndex >= transientPlan.lifetime.firstExecutionIndex)
		{
			return false;
		}

		if (ownerPhysicalPlan.alignment != physicalPlan.alignment || ownerPhysicalPlan.sizeInBytes < physicalPlan.sizeInBytes ||
		    ownerPhysicalPlan.memoryBlockOffset != physicalPlan.memoryBlockOffset)
		{
			return false;
		}

		if (ownerPhysicalPlan.pool == RhiTransientAllocationPool::Buffer)
		{
			if (ownerPhysicalPlan.bufferResourceDesc != physicalPlan.bufferResourceDesc)
			{
				return false;
			}
		}
		else if (ownerPhysicalPlan.textureResourceDesc != physicalPlan.textureResourceDesc)
		{
			return false;
		}

		if (ownerPhysicalPlan.hasOptimizedClearValue != physicalPlan.hasOptimizedClearValue)
		{
			return false;
		}

		if (ownerPhysicalPlan.hasOptimizedClearValue &&
		    !AreClearValuesEqual(ownerPhysicalPlan.optimizedClearValue, physicalPlan.optimizedClearValue, transientPlan.kind))
		{
			return false;
		}

		return true;
	}

};

void FrameGraphCompiler::BuildTransientResourceLifetimes() noexcept
{
	for (FrameGraphTransientResourcePlan& transientPlan : m_plan.transients.resources)
	{
		transientPlan.lifetime.Clear();
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
			if (transientPlan->lifetime.firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX)
			{
				transientPlan->lifetime.firstExecutionIndex = executionIndexValue;
				transientPlan->lifetime.firstUserPass = passIndex;
			}

			transientPlan->lifetime.lastExecutionIndex = executionIndexValue;
			transientPlan->lifetime.lastUserPass = passIndex;

			if (ReadsFromUsage(declaration.usage))
			{
				transientPlan->lifetime.readUsed = true;
			}

			if (WritesToUsage(declaration.usage))
			{
				transientPlan->lifetime.writeUsed = true;
			}

			const ResourceState requiredState = InferRequiredResourceState(declaration, compiledResource);
			const auto stateIt =
			    std::find(transientPlan->lifetime.requiredStates.begin(), transientPlan->lifetime.requiredStates.end(), requiredState);
			if (stateIt == transientPlan->lifetime.requiredStates.end())
			{
				transientPlan->lifetime.requiredStates.push_back(requiredState);
			}
		}
	}

	for (const FrameGraphProductRoot& productRoot : m_plan.productRoots)
	{
		FrameGraphTransientResourcePlan* transientPlan = FindTransientResourcePlan(productRoot.handle);
		if (transientPlan == nullptr || transientPlan->lifetime.firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX)
		{
			continue;
		}

		const auto stateIt =
		    std::find(transientPlan->lifetime.requiredStates.begin(), transientPlan->lifetime.requiredStates.end(), productRoot.requiredState);
		if (stateIt == transientPlan->lifetime.requiredStates.end())
		{
			transientPlan->lifetime.requiredStates.push_back(productRoot.requiredState);
		}
		transientPlan->lifetime.readUsed = true;
		if (!m_plan.executionOrder.empty())
		{
			transientPlan->lifetime.lastExecutionIndex = static_cast<FrameGraphPassIndex>(m_plan.executionOrder.size() - 1);
			transientPlan->lifetime.lastUserPass = m_plan.executionOrder.back();
		}
	}

	const auto unusedIt = std::remove_if(
	    m_plan.transients.resources.begin(),
	    m_plan.transients.resources.end(),
	    [](const FrameGraphTransientResourcePlan& transientPlan) noexcept
	    {
		    return transientPlan.lifetime.firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX;
	    });
	m_plan.transients.resources.erase(unusedIt, m_plan.transients.resources.end());

	for (const FrameGraphTransientResourcePlan& transientPlan : m_plan.transients.resources)
	{
		assert(transientPlan.handle.IsValid());
		assert(transientPlan.lifetime.firstExecutionIndex != INVALID_FRAME_GRAPH_PASS_INDEX);
		assert(transientPlan.lifetime.lastExecutionIndex != INVALID_FRAME_GRAPH_PASS_INDEX);
		assert(transientPlan.lifetime.firstExecutionIndex <= transientPlan.lifetime.lastExecutionIndex);
		assert(transientPlan.lifetime.firstUserPass != INVALID_FRAME_GRAPH_PASS_INDEX);
		assert(transientPlan.lifetime.lastUserPass != INVALID_FRAME_GRAPH_PASS_INDEX);
		assert(!transientPlan.lifetime.requiredStates.empty());
	}
}

void FrameGraphCompiler::BuildTransientPhysicalBlockAssignments() noexcept
{
	m_plan.transients.physicalBlocks.clear();

	std::vector<FrameGraphTransientResourcePlan*> assignmentOrder;
	assignmentOrder.reserve(m_plan.transients.resources.size());
	for (FrameGraphTransientResourcePlan& transientPlan : m_plan.transients.resources)
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

		    if (lhs->lifetime.firstExecutionIndex != rhs->lifetime.firstExecutionIndex)
		    {
			    return lhs->lifetime.firstExecutionIndex < rhs->lifetime.firstExecutionIndex;
		    }

		    return lhs->handle.index < rhs->handle.index;
	    });

	for (FrameGraphTransientResourcePlan* transientPlan : assignmentOrder)
	{
		assert(transientPlan != nullptr);

		FrameGraphTransientPhysicalBlockPlan* selectedBlock = nullptr;
		for (FrameGraphTransientPhysicalBlockPlan& block : m_plan.transients.physicalBlocks)
		{
			assert(!block.handles.empty());
			const FrameGraphTransientResourcePlan* currentOwner = FindTransientResourcePlan(block.handles.back());
			assert(currentOwner != nullptr);
			if (!FrameGraphCompilerTransientsOperations::CanSharePhysicalBlock(*currentOwner, *transientPlan))
			{
				continue;
			}

			selectedBlock = &block;
			break;
		}

		if (selectedBlock == nullptr)
		{
			const std::uint32_t blockIndex = static_cast<std::uint32_t>(m_plan.transients.physicalBlocks.size());
			m_plan.transients.physicalBlocks.push_back(
			    FrameGraphTransientPhysicalBlockPlan{
			        .physicalBlockIndex = blockIndex,
			        .handles = {transientPlan->handle}});
			selectedBlock = &m_plan.transients.physicalBlocks.back();
		}
		else
		{
			selectedBlock->handles.push_back(transientPlan->handle);
		}

		transientPlan->physicalAllocation.physicalBlockIndex = selectedBlock->physicalBlockIndex;
	}

	for (const FrameGraphTransientResourcePlan& transientPlan : m_plan.transients.resources)
	{
		assert(transientPlan.physicalAllocation.physicalBlockIndex != INVALID_FRAME_GRAPH_RESOURCE_INDEX);
	}
}

void FrameGraphCompiler::BuildTransientAliasingBarriers() noexcept
{
	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		passRecord.transientAliasingBarriers.clear();
	}
	m_plan.initialTransientAliasingBarriers.clear();

	for (const FrameGraphTransientPhysicalBlockPlan& block : m_plan.transients.physicalBlocks)
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
			    if (lhs->lifetime.firstExecutionIndex != rhs->lifetime.firstExecutionIndex)
			    {
				    return lhs->lifetime.firstExecutionIndex < rhs->lifetime.firstExecutionIndex;
			    }

			    return lhs->handle.index < rhs->handle.index;
		    });

		for (std::size_t ownerIndex = 1; ownerIndex < orderedPlans.size(); ++ownerIndex)
		{
			const FrameGraphTransientResourcePlan& previousOwner = *orderedPlans[ownerIndex - 1];
			const FrameGraphTransientResourcePlan& nextOwner = *orderedPlans[ownerIndex];

			assert(previousOwner.lifetime.lastExecutionIndex < nextOwner.lifetime.firstExecutionIndex);

			FrameGraphAliasingBarrier barrier{
			    .physicalBlockIndex = block.physicalBlockIndex,
			    .beforeHandle = previousOwner.handle,
			    .afterHandle = nextOwner.handle,
			    .executeBeforePass = previousOwner.lifetime.lastUserPass,
			    .executeAfterPass = nextOwner.lifetime.firstUserPass};

			assert(barrier.executeAfterPass != INVALID_FRAME_GRAPH_PASS_INDEX);
			assert(barrier.executeAfterPass < m_plan.passes.size());
			m_plan.passes[barrier.executeAfterPass].transientAliasingBarriers.push_back(barrier);
		}

		const FrameGraphTransientResourcePlan& lastOwner = *orderedPlans.back();
		const FrameGraphTransientResourcePlan& firstOwner = *orderedPlans.front();
		m_plan.initialTransientAliasingBarriers.push_back(
		    FrameGraphAliasingBarrier{
		        .physicalBlockIndex = block.physicalBlockIndex,
		        .beforeHandle = lastOwner.handle,
		        .afterHandle = firstOwner.handle,
		        .executeBeforePass = lastOwner.lifetime.lastUserPass,
		        .executeAfterPass = firstOwner.lifetime.firstUserPass});
	}
}
