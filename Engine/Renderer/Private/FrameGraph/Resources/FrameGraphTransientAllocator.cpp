#include "PCH.h"
#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <algorithm>
#include <cassert>
#include <string>

namespace
{
	bool RequiresShaderResourceView(const FrameGraphTransientResourcePlan& transientPlan) noexcept
	{
		if (transientPlan.kind == FrameGraphResourceKind::ColorRenderTarget)
		{
			return true;
		}

		return transientPlan.kind != FrameGraphResourceKind::DepthStencil &&
		       std::find(
		           transientPlan.lifetime.requiredStates.begin(),
		           transientPlan.lifetime.requiredStates.end(),
		           ResourceState::ShaderResource) != transientPlan.lifetime.requiredStates.end();
	}

	bool RequiresUnorderedAccessView(const FrameGraphTransientResourcePlan& transientPlan) noexcept
	{
		return transientPlan.kind != FrameGraphResourceKind::DepthStencil &&
		       std::find(
		           transientPlan.lifetime.requiredStates.begin(),
		           transientPlan.lifetime.requiredStates.end(),
		           ResourceState::UnorderedAccess) != transientPlan.lifetime.requiredStates.end();
	}

	std::wstring BuildWideDebugName(const std::string& name, const wchar_t* fallbackName)
	{
		if (name.empty())
		{
			return fallbackName;
		}

		return std::wstring(name.begin(), name.end());
	}

	std::wstring BuildMemoryBlockDebugName(const FrameGraphTransientResourcePlan& transientPlan)
	{
		const std::string& debugName =
		    transientPlan.resourceClass == FrameGraphResourceClass::Buffer ? transientPlan.bufferDesc.name : transientPlan.textureDesc.name;
		std::wstring memoryBlockName = BuildWideDebugName(debugName, L"FG_Transient");
		memoryBlockName += L"_MemoryBlock";
		memoryBlockName += std::to_wstring(transientPlan.physicalAllocation.physicalBlockIndex);
		return memoryBlockName;
	}
}

FrameGraphTransientAllocator::FrameGraphTransientAllocator(RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface)
{
}

void FrameGraphTransientAllocator::Reset() noexcept
{
	ReleaseAllocationDescriptors(m_colorAllocations);
	ReleaseAllocationDescriptors(m_depthAllocations);
	ReleaseAllocationDescriptors(m_bufferAllocations);
	m_colorAllocations.clear();
	m_depthAllocations.clear();
	m_bufferAllocations.clear();
	m_colorBlocks.clear();
	m_depthBlocks.clear();
	m_bufferBlocks.clear();
}

void FrameGraphTransientAllocator::ReleaseAllocationDescriptors(AllocationList& allocations) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	for (AllocationRecord& allocation : allocations)
	{
		if (allocation.renderTargetView)
		{
			m_renderHardwareInterface->ReleaseResourceView(allocation.renderTargetView);
			allocation.renderTargetView = {};
		}

		if (allocation.depthStencilView)
		{
			m_renderHardwareInterface->ReleaseResourceView(allocation.depthStencilView);
			allocation.depthStencilView = {};
		}

		if (allocation.shaderResourceView)
		{
			m_renderHardwareInterface->ReleaseResourceView(allocation.shaderResourceView);
			allocation.shaderResourceView = {};
		}

		if (allocation.unorderedAccessView)
		{
			m_renderHardwareInterface->ReleaseResourceView(allocation.unorderedAccessView);
			allocation.unorderedAccessView = {};
		}

		if (allocation.ownedDepthStencilResource)
		{
			m_renderHardwareInterface->ReleaseOwnedResource(allocation.ownedDepthStencilResource);
			allocation.ownedDepthStencilResource = {};
		}

		if (allocation.ownedRenderTargetResource)
		{
			m_renderHardwareInterface->ReleaseOwnedResource(allocation.ownedRenderTargetResource);
			allocation.ownedRenderTargetResource = {};
		}

		if (allocation.ownedBuffer)
		{
			m_renderHardwareInterface->ReleaseOwnedResource(allocation.ownedBuffer);
			allocation.ownedBuffer = {};
		}

		allocation.depthStencilResource = {};
		allocation.renderTargetResource = {};
		allocation.buffer = {};
	}

	for (PhysicalBlockRecord& block : m_colorBlocks)
	{
		if (block.ownedMemoryBlock)
		{
			m_renderHardwareInterface->ReleaseTransientMemoryBlock(block.ownedMemoryBlock);
			block.ownedMemoryBlock = {};
		}
	}

	for (PhysicalBlockRecord& block : m_depthBlocks)
	{
		if (block.ownedMemoryBlock)
		{
			m_renderHardwareInterface->ReleaseTransientMemoryBlock(block.ownedMemoryBlock);
			block.ownedMemoryBlock = {};
		}
	}

	for (PhysicalBlockRecord& block : m_bufferBlocks)
	{
		if (block.ownedMemoryBlock)
		{
			m_renderHardwareInterface->ReleaseTransientMemoryBlock(block.ownedMemoryBlock);
			block.ownedMemoryBlock = {};
		}
	}
}

FrameGraphTransientAllocator::AllocationRecord& FrameGraphTransientAllocator::Materialize(
    const FrameGraphTransientResourcePlan& transientPlan)
{
	AllocationList& allocations = GetAllocationList(transientPlan.physicalAllocation.pool);
	if (AllocationRecord* existingAllocation = const_cast<AllocationRecord*>(FindAllocationInList(allocations, transientPlan.handle)))
	{
		return *existingAllocation;
	}

	allocations.push_back(CreateAllocationRecord(transientPlan));
	return allocations.back();
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindAllocation(FrameGraphResourceHandle handle) const noexcept
{
	if (const AllocationRecord* depthAllocation = FindDepthAllocation(handle))
	{
		return depthAllocation;
	}

	if (const AllocationRecord* bufferAllocation = FindBufferAllocation(handle))
	{
		return bufferAllocation;
	}

	return FindColorAllocation(handle);
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindDepthAllocation(
    FrameGraphResourceHandle handle) const noexcept
{
	return FindAllocationInList(m_depthAllocations, handle);
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindColorAllocation(
    FrameGraphResourceHandle handle) const noexcept
{
	return FindAllocationInList(m_colorAllocations, handle);
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindBufferAllocation(
    FrameGraphResourceHandle handle) const noexcept
{
	return FindAllocationInList(m_bufferAllocations, handle);
}

FrameGraphTransientAllocator::BlockList& FrameGraphTransientAllocator::GetBlockList(
    FrameGraphTransientResourcePlan::AllocationPool pool) noexcept
{
	switch (pool)
	{
		case FrameGraphTransientResourcePlan::AllocationPool::Depth:
			return m_depthBlocks;
		case FrameGraphTransientResourcePlan::AllocationPool::Buffer:
			return m_bufferBlocks;
		default:
			return m_colorBlocks;
	}
}

const FrameGraphTransientAllocator::BlockList& FrameGraphTransientAllocator::GetBlockList(
    FrameGraphTransientResourcePlan::AllocationPool pool) const noexcept
{
	switch (pool)
	{
		case FrameGraphTransientResourcePlan::AllocationPool::Depth:
			return m_depthBlocks;
		case FrameGraphTransientResourcePlan::AllocationPool::Buffer:
			return m_bufferBlocks;
		default:
			return m_colorBlocks;
	}
}

FrameGraphTransientAllocator::AllocationList& FrameGraphTransientAllocator::GetAllocationList(
    FrameGraphTransientResourcePlan::AllocationPool pool) noexcept
{
	switch (pool)
	{
		case FrameGraphTransientResourcePlan::AllocationPool::Depth:
			return m_depthAllocations;
		case FrameGraphTransientResourcePlan::AllocationPool::Buffer:
			return m_bufferAllocations;
		default:
			return m_colorAllocations;
	}
}

const FrameGraphTransientAllocator::AllocationList& FrameGraphTransientAllocator::GetAllocationList(
    FrameGraphTransientResourcePlan::AllocationPool pool) const noexcept
{
	switch (pool)
	{
		case FrameGraphTransientResourcePlan::AllocationPool::Depth:
			return m_depthAllocations;
		case FrameGraphTransientResourcePlan::AllocationPool::Buffer:
			return m_bufferAllocations;
		default:
			return m_colorAllocations;
	}
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindAllocationInList(
    const AllocationList& allocations,
    FrameGraphResourceHandle handle) const noexcept
{
	const auto it = std::find_if(
	    allocations.begin(),
	    allocations.end(),
	    [handle](const AllocationRecord& allocation)
	    {
		    return allocation.handle == handle;
	    });

	return it != allocations.end() ? &(*it) : nullptr;
}

FrameGraphTransientAllocator::PhysicalBlockRecord* FrameGraphTransientAllocator::FindPhysicalBlock(
    BlockList& blocks,
    std::uint32_t physicalBlockIndex) noexcept
{
	const auto it = std::find_if(
	    blocks.begin(),
	    blocks.end(),
	    [physicalBlockIndex](const PhysicalBlockRecord& block)
	    {
		    return block.physicalBlockIndex == physicalBlockIndex;
	    });

	return it != blocks.end() ? &(*it) : nullptr;
}

FrameGraphTransientAllocator::PhysicalBlockRecord& FrameGraphTransientAllocator::GetOrCreatePhysicalBlock(
    const FrameGraphTransientResourcePlan& transientPlan)
{
	assert(m_renderHardwareInterface != nullptr);
	assert(transientPlan.physicalAllocation.physicalBlockIndex != INVALID_FRAME_GRAPH_RESOURCE_INDEX);
	BlockList& blocks = GetBlockList(transientPlan.physicalAllocation.pool);
	if (PhysicalBlockRecord* existingBlock = FindPhysicalBlock(blocks, transientPlan.physicalAllocation.physicalBlockIndex))
	{
		return *existingBlock;
	}

	PhysicalBlockRecord block;
	block.physicalBlockIndex = transientPlan.physicalAllocation.physicalBlockIndex;
	block.pool = transientPlan.physicalAllocation.pool;
	block.sizeInBytes = transientPlan.physicalAllocation.sizeInBytes;
	block.alignment = transientPlan.physicalAllocation.alignment;
	block.memoryBlockOffset = transientPlan.physicalAllocation.memoryBlockOffset;
	block.ownedMemoryBlock = m_renderHardwareInterface->CreateTransientMemoryBlock(
	    block.pool == FrameGraphTransientResourcePlan::AllocationPool::Buffer
	        ? RhiTransientAllocationPool::Buffer
	        : (block.pool == FrameGraphTransientResourcePlan::AllocationPool::Depth ? RhiTransientAllocationPool::Depth
	                                                                                          : RhiTransientAllocationPool::Color),
	    block.sizeInBytes,
	    block.alignment,
	    BuildMemoryBlockDebugName(transientPlan));
	assert(block.ownedMemoryBlock);

	blocks.push_back(std::move(block));
	return blocks.back();
}

FrameGraphTransientAllocator::AllocationRecord FrameGraphTransientAllocator::CreateAllocationRecord(
    const FrameGraphTransientResourcePlan& transientPlan)
{
	assert(m_renderHardwareInterface != nullptr);
	assert(transientPlan.handle.IsValid());
	assert(transientPlan.physicalAllocation.allocationIndex != INVALID_FRAME_GRAPH_RESOURCE_INDEX);
	assert(transientPlan.physicalAllocation.physicalBlockIndex != INVALID_FRAME_GRAPH_RESOURCE_INDEX);
	assert(transientPlan.physicalAllocation.sizeInBytes > 0);
	assert(transientPlan.physicalAllocation.alignment > 0);

	AllocationRecord allocation;
	allocation.handle = transientPlan.handle;
	allocation.kind = transientPlan.kind;
	allocation.allocationIndex = transientPlan.physicalAllocation.allocationIndex;
	allocation.physicalBlockIndex = transientPlan.physicalAllocation.physicalBlockIndex;
	allocation.sizeInBytes = transientPlan.physicalAllocation.sizeInBytes;
	allocation.alignment = transientPlan.physicalAllocation.alignment;
	allocation.memoryBlockOffset = transientPlan.physicalAllocation.memoryBlockOffset;

	PhysicalBlockRecord& block = GetOrCreatePhysicalBlock(transientPlan);
	assert(block.ownedMemoryBlock);

	switch (transientPlan.kind)
	{
		case FrameGraphResourceKind::DepthStencil:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.textureDesc.name, L"FG_DepthTransient");
			allocation.ownedDepthStencilResource = m_renderHardwareInterface->CreateAliasingTextureResource(
			    block.ownedMemoryBlock,
			    allocation.memoryBlockOffset,
			    RhiTransientTextureAllocationDesc{
			        .ResourceDesc = transientPlan.physicalAllocation.textureResourceDesc,
			        .ClearValue = transientPlan.physicalAllocation.optimizedClearValue,
			        .InitialState = transientPlan.physicalAllocation.initialState},
			    debugName);
			allocation.depthStencilResource = m_renderHardwareInterface->GetNativeResource(allocation.ownedDepthStencilResource);
			allocation.depthStencilView = m_renderHardwareInterface->CreateResourceView(
			    RhiResourceViewDesc::DepthStencil(allocation.depthStencilResource, transientPlan.textureDesc.format));
			break;
		}

		case FrameGraphResourceKind::ColorRenderTarget:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.textureDesc.name, L"FG_ColorTransient");
			allocation.ownedRenderTargetResource = m_renderHardwareInterface->CreateAliasingTextureResource(
			    block.ownedMemoryBlock,
			    allocation.memoryBlockOffset,
			    RhiTransientTextureAllocationDesc{
			        .ResourceDesc = transientPlan.physicalAllocation.textureResourceDesc,
			        .ClearValue = transientPlan.physicalAllocation.optimizedClearValue,
			        .InitialState = transientPlan.physicalAllocation.initialState},
			    debugName);
			allocation.renderTargetResource = m_renderHardwareInterface->GetNativeResource(allocation.ownedRenderTargetResource);
			allocation.renderTargetView = m_renderHardwareInterface->CreateResourceView(
			    RhiResourceViewDesc::RenderTarget(allocation.renderTargetResource, transientPlan.textureDesc.format));

			if (RequiresShaderResourceView(transientPlan))
			{
				allocation.shaderResourceView = m_renderHardwareInterface->CreateResourceView(
				    RhiResourceViewDesc::TextureShaderResource(allocation.renderTargetResource, transientPlan.textureDesc.format));
			}

			if (RequiresUnorderedAccessView(transientPlan))
			{
				allocation.unorderedAccessView = m_renderHardwareInterface->CreateResourceView(
				    RhiResourceViewDesc::TextureUnorderedAccess(allocation.renderTargetResource, transientPlan.textureDesc.format));
			}
			break;
		}

		case FrameGraphResourceKind::Buffer:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.bufferDesc.name, L"FG_BufferTransient");
			allocation.ownedBuffer = m_renderHardwareInterface->CreateAliasingBufferResource(
			    block.ownedMemoryBlock,
			    allocation.memoryBlockOffset,
			    RhiTransientBufferAllocationDesc{
			        .ResourceDesc = transientPlan.physicalAllocation.bufferResourceDesc,
			        .InitialState = transientPlan.physicalAllocation.initialState},
			    debugName);
			allocation.buffer = m_renderHardwareInterface->GetNativeResource(allocation.ownedBuffer);

			if (RequiresShaderResourceView(transientPlan))
			{
				allocation.shaderResourceView = m_renderHardwareInterface->CreateResourceView(RhiResourceViewDesc::BufferShaderResource(
				    allocation.buffer,
				    transientPlan.bufferDesc.sizeInBytes,
				    transientPlan.bufferDesc.strideInBytes));
			}

			if (RequiresUnorderedAccessView(transientPlan))
			{
				allocation.unorderedAccessView = m_renderHardwareInterface->CreateResourceView(RhiResourceViewDesc::BufferUnorderedAccess(
				    allocation.buffer,
				    transientPlan.bufferDesc.sizeInBytes,
				    transientPlan.bufferDesc.strideInBytes));
			}
			break;
		}

		default:
			Diagnostics::Fail(
			    Logging::GetOrCreateLogger("Renderer.FrameGraph"),
			    __FILE__,
			    __LINE__,
			    "FrameGraphTransientAllocator: unsupported transient resource kind for heap-backed allocation");
			break;
	}

	return allocation;
}
