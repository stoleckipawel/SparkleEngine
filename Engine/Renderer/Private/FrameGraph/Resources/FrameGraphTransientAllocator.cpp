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
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(allocation.renderTargetView);
			allocation.renderTargetView = {};
		}

		if (allocation.depthStencilView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(allocation.depthStencilView);
			allocation.depthStencilView = {};
		}

		if (allocation.shaderResourceView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(allocation.shaderResourceView);
			allocation.shaderResourceView = {};
		}

		if (allocation.unorderedAccessView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(allocation.unorderedAccessView);
			allocation.unorderedAccessView = {};
		}

		if (allocation.ownedDepthStencilResource)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(allocation.ownedDepthStencilResource);
			allocation.ownedDepthStencilResource = {};
		}

		if (allocation.ownedRenderTargetResource)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(allocation.ownedRenderTargetResource);
			allocation.ownedRenderTargetResource = {};
		}

		if (allocation.ownedBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(allocation.ownedBuffer);
			allocation.ownedBuffer = {};
		}

		allocation.depthStencilResource = {};
		allocation.renderTargetResource = {};
		allocation.buffer = {};
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

FrameGraphTransientAllocator::AllocationRecord FrameGraphTransientAllocator::CreateAllocationRecord(
    const FrameGraphTransientResourcePlan& transientPlan)
{
	assert(m_renderHardwareInterface != nullptr);
	assert(transientPlan.handle.IsValid());
	assert(transientPlan.physicalAllocation.sizeInBytes > 0);
	assert(transientPlan.physicalAllocation.alignment > 0);

	AllocationRecord allocation;
	allocation.handle = transientPlan.handle;
	allocation.kind = transientPlan.kind;

	switch (transientPlan.kind)
	{
		case FrameGraphResourceKind::DepthStencil:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.textureDesc.name, L"FG_DepthTransient");
			allocation.ownedDepthStencilResource = m_renderHardwareInterface->GetResourceService().CreateTextureResource(
			    transientPlan.physicalAllocation.textureResourceDesc,
			    transientPlan.physicalAllocation.initialState,
			    RhiMemoryCategory::TransientResource,
			    RhiMemoryResidencyClass::Transient,
			    debugName);
			allocation.depthStencilResource = m_renderHardwareInterface->GetResourceService().GetNativeResource(allocation.ownedDepthStencilResource);
			allocation.depthStencilView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
			    RhiResourceViewDesc::DepthStencil(allocation.depthStencilResource, transientPlan.textureDesc.format));
			break;
		}

		case FrameGraphResourceKind::ColorRenderTarget:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.textureDesc.name, L"FG_ColorTransient");
			allocation.ownedRenderTargetResource = m_renderHardwareInterface->GetResourceService().CreateTextureResource(
			    transientPlan.physicalAllocation.textureResourceDesc,
			    transientPlan.physicalAllocation.initialState,
			    RhiMemoryCategory::TransientResource,
			    RhiMemoryResidencyClass::Transient,
			    debugName);
			allocation.renderTargetResource = m_renderHardwareInterface->GetResourceService().GetNativeResource(allocation.ownedRenderTargetResource);
			if (transientPlan.physicalAllocation.textureResourceDesc.AllowRenderTarget)
			{
				allocation.renderTargetView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
				    RhiResourceViewDesc::RenderTarget(allocation.renderTargetResource, transientPlan.textureDesc.format));
			}

			if (RequiresShaderResourceView(transientPlan))
			{
				allocation.shaderResourceView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
				    RhiResourceViewDesc::TextureShaderResource(allocation.renderTargetResource, transientPlan.textureDesc.format));
			}

			if (RequiresUnorderedAccessView(transientPlan))
			{
				allocation.unorderedAccessView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
				    RhiResourceViewDesc::TextureUnorderedAccess(allocation.renderTargetResource, transientPlan.textureDesc.format));
			}
			break;
		}

		case FrameGraphResourceKind::Buffer:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.bufferDesc.name, L"FG_BufferTransient");
			allocation.ownedBuffer = m_renderHardwareInterface->GetResourceService().CreateBufferResource(
			    transientPlan.physicalAllocation.bufferResourceDesc,
			    transientPlan.physicalAllocation.initialState,
			    RhiMemoryCategory::TransientResource,
			    RhiMemoryResidencyClass::Transient,
			    debugName);
			allocation.buffer = m_renderHardwareInterface->GetResourceService().GetNativeResource(allocation.ownedBuffer);

			if (RequiresShaderResourceView(transientPlan))
			{
				allocation.shaderResourceView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(RhiResourceViewDesc::BufferShaderResource(
				    allocation.buffer,
				    transientPlan.bufferDesc.sizeInBytes,
				    transientPlan.bufferDesc.strideInBytes));
			}

			if (RequiresUnorderedAccessView(transientPlan))
			{
				allocation.unorderedAccessView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(RhiResourceViewDesc::BufferUnorderedAccess(
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
