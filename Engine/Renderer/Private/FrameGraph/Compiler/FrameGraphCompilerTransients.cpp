#include "PCH.h"
#include "FrameGraphCompiler.h"

#include <algorithm>
#include <cassert>

namespace
{
	using AllocationPool = FrameGraph::CompiledTransientResourcePlan::AllocationPool;

	bool AreResourceDescsEqual(const D3D12_RESOURCE_DESC& lhs, const D3D12_RESOURCE_DESC& rhs) noexcept
	{
		return lhs.Dimension == rhs.Dimension && lhs.Alignment == rhs.Alignment && lhs.Width == rhs.Width &&
		       lhs.Height == rhs.Height && lhs.DepthOrArraySize == rhs.DepthOrArraySize && lhs.MipLevels == rhs.MipLevels &&
		       lhs.Format == rhs.Format && lhs.SampleDesc.Count == rhs.SampleDesc.Count &&
		       lhs.SampleDesc.Quality == rhs.SampleDesc.Quality && lhs.Layout == rhs.Layout && lhs.Flags == rhs.Flags;
	}

	bool AreClearValuesEqual(const D3D12_CLEAR_VALUE& lhs, const D3D12_CLEAR_VALUE& rhs, FrameGraphResourceKind kind) noexcept
	{
		if (lhs.Format != rhs.Format)
		{
			return false;
		}

		if (kind == FrameGraphResourceKind::DepthStencil)
		{
			return lhs.DepthStencil.Depth == rhs.DepthStencil.Depth && lhs.DepthStencil.Stencil == rhs.DepthStencil.Stencil;
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

	bool CanSharePhysicalBlock(const FrameGraph::CompiledPhysicalBlockPlan& block,
	                           const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		const auto& physicalPlan = transientPlan.physicalAllocation;
		if (block.pool != physicalPlan.pool)
		{
			return false;
		}

		if (block.lastExecutionIndex == FrameGraph::INVALID_PASS_INDEX || transientPlan.firstExecutionIndex == FrameGraph::INVALID_PASS_INDEX)
		{
			return false;
		}

		if (block.lastExecutionIndex >= transientPlan.firstExecutionIndex)
		{
			return false;
		}

		if (block.heapFlags != physicalPlan.heapFlags || block.alignment != physicalPlan.alignment ||
		    block.sizeInBytes < physicalPlan.sizeInBytes || block.heapOffset != physicalPlan.heapOffset)
		{
			return false;
		}

		if (!AreResourceDescsEqual(block.resourceDesc, physicalPlan.resourceDesc))
		{
			return false;
		}

		if (block.hasOptimizedClearValue != physicalPlan.hasOptimizedClearValue)
		{
			return false;
		}

		if (block.hasOptimizedClearValue && !AreClearValuesEqual(block.optimizedClearValue, physicalPlan.optimizedClearValue, transientPlan.kind))
		{
			return false;
		}

		return true;
	}
}

void FrameGraphCompiler::BuildTransientResourceLifetimes() noexcept
{
	for (FrameGraph::CompiledTransientResourcePlan& transientPlan : m_plan.transientResources)
	{
		transientPlan.firstUserPass = FrameGraph::INVALID_PASS_INDEX;
		transientPlan.lastUserPass = FrameGraph::INVALID_PASS_INDEX;
		transientPlan.firstExecutionIndex = FrameGraph::INVALID_PASS_INDEX;
		transientPlan.lastExecutionIndex = FrameGraph::INVALID_PASS_INDEX;
		transientPlan.readUsed = false;
		transientPlan.writeUsed = false;
		transientPlan.requiredStates.clear();
	}

	for (std::size_t executionIndex = 0; executionIndex < m_plan.executionOrder.size(); ++executionIndex)
	{
		const PassIndex passIndex = m_plan.executionOrder[executionIndex];
		const CompilePassRecord& passRecord = m_plan.passes[passIndex];

		for (const PassResourceDeclaration& declaration : passRecord.declarations)
		{
			if (!declaration.handle.IsValid())
			{
				continue;
			}

			FrameGraph::CompiledTransientResourcePlan* transientPlan = FindTransientResourcePlan(declaration.handle);
			if (transientPlan == nullptr)
			{
				continue;
			}

			const CompileResourceEntry& compiledResource = GetCompiledResourceEntry(declaration.handle);
			assert(compiledResource.ownership == FrameGraphResourceOwnership::Transient);

			const PassIndex executionIndexValue = static_cast<PassIndex>(executionIndex);
			if (transientPlan->firstExecutionIndex == FrameGraph::INVALID_PASS_INDEX)
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

	for (const FrameGraph::CompiledTransientResourcePlan& transientPlan : m_plan.transientResources)
	{
		assert(transientPlan.handle.IsValid());
		assert(transientPlan.firstExecutionIndex != FrameGraph::INVALID_PASS_INDEX);
		assert(transientPlan.lastExecutionIndex != FrameGraph::INVALID_PASS_INDEX);
		assert(transientPlan.firstExecutionIndex <= transientPlan.lastExecutionIndex);
		assert(transientPlan.firstUserPass != FrameGraph::INVALID_PASS_INDEX);
		assert(transientPlan.lastUserPass != FrameGraph::INVALID_PASS_INDEX);
		assert(!transientPlan.requiredStates.empty());

	}
}

void FrameGraphCompiler::BuildTransientPhysicalBlockAssignments() noexcept
{
	m_plan.physicalBlocks.clear();

	std::vector<FrameGraph::CompiledTransientResourcePlan*> assignmentOrder;
	assignmentOrder.reserve(m_plan.transientResources.size());
	for (FrameGraph::CompiledTransientResourcePlan& transientPlan : m_plan.transientResources)
	{
		transientPlan.physicalAllocation.physicalBlockIndex = FrameGraph::INVALID_RESOURCE_INDEX;
		assignmentOrder.push_back(&transientPlan);
	}

	std::sort(
	    assignmentOrder.begin(),
	    assignmentOrder.end(),
	    [](const FrameGraph::CompiledTransientResourcePlan* lhs, const FrameGraph::CompiledTransientResourcePlan* rhs)
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

	for (FrameGraph::CompiledTransientResourcePlan* transientPlan : assignmentOrder)
	{
		assert(transientPlan != nullptr);

		FrameGraph::CompiledPhysicalBlockPlan* selectedBlock = nullptr;
		for (FrameGraph::CompiledPhysicalBlockPlan& block : m_plan.physicalBlocks)
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
			const std::string blockLabel = std::string{"Block#"} + std::to_string(blockIndex);
			m_plan.physicalBlocks.push_back(
			    FrameGraph::CompiledPhysicalBlockPlan{.physicalBlockIndex = blockIndex,
			                                          .pool = transientPlan->physicalAllocation.pool,
			                                          .sizeInBytes = transientPlan->physicalAllocation.sizeInBytes,
			                                          .alignment = transientPlan->physicalAllocation.alignment,
			                                          .heapOffset = transientPlan->physicalAllocation.heapOffset,
			                                          .heapFlags = transientPlan->physicalAllocation.heapFlags,
			                                          .resourceDesc = transientPlan->physicalAllocation.resourceDesc,
			                                          .optimizedClearValue = transientPlan->physicalAllocation.optimizedClearValue,
			                                          .hasOptimizedClearValue = transientPlan->physicalAllocation.hasOptimizedClearValue,
			                                          .firstExecutionIndex = transientPlan->firstExecutionIndex,
			                                          .lastExecutionIndex = transientPlan->lastExecutionIndex,
			                                          .displayLabel = blockLabel,
			                                          .eventScopeLabel = std::string{"FG/PhysicalBlock/"} + std::to_string(blockIndex),
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

	for (const FrameGraph::CompiledTransientResourcePlan& transientPlan : m_plan.transientResources)
	{
		assert(transientPlan.physicalAllocation.physicalBlockIndex != FrameGraph::INVALID_RESOURCE_INDEX);
	}
}

void FrameGraphCompiler::BuildTransientAliasingBarriers() noexcept
{
	for (CompilePassRecord& passRecord : m_plan.passes)
	{
		passRecord.compiledAliasingBarriers.clear();
	}
	m_plan.finalAliasingBarriers.clear();

	for (const FrameGraph::CompiledPhysicalBlockPlan& block : m_plan.physicalBlocks)
	{
		if (block.handles.size() < 2)
		{
			continue;
		}

		std::vector<const FrameGraph::CompiledTransientResourcePlan*> orderedPlans;
		orderedPlans.reserve(block.handles.size());
		for (const ResourceHandle handle : block.handles)
		{
			const FrameGraph::CompiledTransientResourcePlan* transientPlan = FindTransientResourcePlan(handle);
			assert(transientPlan != nullptr);
			orderedPlans.push_back(transientPlan);
		}

		std::sort(
		    orderedPlans.begin(),
		    orderedPlans.end(),
		    [](const FrameGraph::CompiledTransientResourcePlan* lhs, const FrameGraph::CompiledTransientResourcePlan* rhs)
		    {
			    if (lhs->firstExecutionIndex != rhs->firstExecutionIndex)
			    {
				    return lhs->firstExecutionIndex < rhs->firstExecutionIndex;
			    }

			    return lhs->handle.index < rhs->handle.index;
		    });

		for (std::size_t ownerIndex = 1; ownerIndex < orderedPlans.size(); ++ownerIndex)
		{
			const FrameGraph::CompiledTransientResourcePlan& previousOwner = *orderedPlans[ownerIndex - 1];
			const FrameGraph::CompiledTransientResourcePlan& nextOwner = *orderedPlans[ownerIndex];

			assert(previousOwner.lastExecutionIndex < nextOwner.firstExecutionIndex);

			FrameGraph::CompiledAliasingBarrier barrier{.physicalBlockIndex = block.physicalBlockIndex,
			                                            .beforeHandle = previousOwner.handle,
			                                            .afterHandle = nextOwner.handle,
			                                            .executeBeforePass = previousOwner.lastUserPass,
			                                            .executeAfterPass = nextOwner.firstUserPass};

			assert(barrier.executeAfterPass != FrameGraph::INVALID_PASS_INDEX);
			assert(barrier.executeAfterPass < m_plan.passes.size());
			m_plan.passes[barrier.executeAfterPass].compiledAliasingBarriers.push_back(barrier);

		}
	}
}
