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
	    const FrameGraphTransientPhysicalBlockPlan& block,
	    const FrameGraphTransientResourcePlan& transientPlan) noexcept
	{
		const auto& physicalPlan = transientPlan.physicalAllocation;
		if (block.pool != physicalPlan.pool)
		{
			return false;
		}

		if (block.lastExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX ||
		    transientPlan.lifetime.firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX)
		{
			return false;
		}

		if (block.lastExecutionIndex >= transientPlan.lifetime.firstExecutionIndex)
		{
			return false;
		}

		if (block.alignment != physicalPlan.alignment || block.sizeInBytes < physicalPlan.sizeInBytes ||
		    block.memoryBlockOffset != physicalPlan.memoryBlockOffset)
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

	void ExtendTransientLifetimesToFrame(
	    FrameGraphTransientPlan& transientPlan,
	    const std::vector<FrameGraphPassIndex>& executionOrder) noexcept
	{
		if (!transientPlan.options.extendLifetimesToFrame || executionOrder.empty())
		{
			return;
		}

		const FrameGraphPassIndex firstExecutionIndex = 0;
		const FrameGraphPassIndex lastExecutionIndex = static_cast<FrameGraphPassIndex>(executionOrder.size() - 1);
		const FrameGraphPassIndex firstUserPass = executionOrder.front();
		const FrameGraphPassIndex lastUserPass = executionOrder.back();

		for (FrameGraphTransientResourcePlan& resourcePlan : transientPlan.resources)
		{
			if (resourcePlan.lifetime.firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX)
			{
				continue;
			}

			resourcePlan.lifetime.firstExecutionIndex = firstExecutionIndex;
			resourcePlan.lifetime.lastExecutionIndex = lastExecutionIndex;
			resourcePlan.lifetime.firstUserPass = firstUserPass;
			resourcePlan.lifetime.lastUserPass = lastUserPass;
		}
	}
}  // namespace

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
	}

	ExtendTransientLifetimesToFrame(m_plan.transients, m_plan.executionOrder);

	const auto unusedIt = std::remove_if(
	    m_plan.transients.resources.begin(),
	    m_plan.transients.resources.end(),
	    [](const FrameGraphTransientResourcePlan& transientPlan) noexcept
	    {
		    return transientPlan.lifetime.firstExecutionIndex == INVALID_FRAME_GRAPH_PASS_INDEX;
	    });
	m_plan.transients.resources.erase(unusedIt, m_plan.transients.resources.end());

	for (std::size_t transientIndex = 0; transientIndex < m_plan.transients.resources.size(); ++transientIndex)
	{
		m_plan.transients.resources[transientIndex].physicalAllocation.allocationIndex =
		    static_cast<std::uint32_t>(transientIndex);
	}

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
			if (!m_plan.transients.options.enableAliasing || !CanSharePhysicalBlock(block, *transientPlan))
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
			        .pool = transientPlan->physicalAllocation.pool,
			        .sizeInBytes = transientPlan->physicalAllocation.sizeInBytes,
			        .alignment = transientPlan->physicalAllocation.alignment,
			        .memoryBlockOffset = transientPlan->physicalAllocation.memoryBlockOffset,
			        .textureResourceDesc = transientPlan->physicalAllocation.textureResourceDesc,
			        .bufferResourceDesc = transientPlan->physicalAllocation.bufferResourceDesc,
			        .optimizedClearValue = transientPlan->physicalAllocation.optimizedClearValue,
			        .hasOptimizedClearValue = transientPlan->physicalAllocation.hasOptimizedClearValue,
			        .firstExecutionIndex = transientPlan->lifetime.firstExecutionIndex,
			        .lastExecutionIndex = transientPlan->lifetime.lastExecutionIndex,
			        .handles = {transientPlan->handle}});
			selectedBlock = &m_plan.transients.physicalBlocks.back();
		}
		else
		{
			selectedBlock->lastExecutionIndex = transientPlan->lifetime.lastExecutionIndex;
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
	m_plan.finalTransientAliasingBarriers.clear();
	if (!m_plan.transients.options.enableAliasing)
	{
		return;
	}

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
	}
}
