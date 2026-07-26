#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "Config/DepthConvention.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cassert>
#include <string>

class FrameGraphTransientResourcePlanner final
{
  public:
	static bool RequiresUnorderedAccess(const FrameGraphPlan& plan, FrameGraphResourceHandle handle) noexcept
	{
		for (const FrameGraphPassNode& passRecord : plan.passes)
		{
			for (const PassResourceDeclaration& declaration : passRecord.declarations)
			{
				if (declaration.handle == handle && UsesUnorderedAccess(declaration.usage))
				{
					return true;
				}
			}
		}

		return false;
	}

	static bool RequiresRenderTarget(const FrameGraphPlan& plan, FrameGraphResourceHandle handle) noexcept
	{
		for (const FrameGraphPassNode& passRecord : plan.passes)
		{
			for (const PassResourceDeclaration& declaration : passRecord.declarations)
			{
				if (declaration.handle == handle && declaration.usage == ResourceUsage::RenderTarget)
				{
					return true;
				}
			}
		}

		return false;
	}

	static RhiBufferResourceDesc BuildTransientBufferDesc(const FrameGraphBufferDesc& desc, bool requiresUnorderedAccess) noexcept
	{
		return RhiBufferResourceDesc{
		    .SizeInBytes = desc.sizeInBytes,
		    .StrideInBytes = desc.strideInBytes,
		    .AllowUnorderedAccess = requiresUnorderedAccess};
	}

	static RhiTextureResourceDesc BuildTransientResourceDesc(
	    const FrameGraphTextureDesc& desc,
	    FrameGraphResourceKind kind,
	    bool requiresRenderTarget,
	    bool requiresUnorderedAccess) noexcept
	{
		RhiTextureResourceDesc resourceDesc{};
		resourceDesc.Width = desc.width;
		resourceDesc.Height = desc.height;
		resourceDesc.Format = desc.format;
		resourceDesc.MipLevels = 1;
		resourceDesc.AllowRenderTarget = kind == FrameGraphResourceKind::ColorRenderTarget && requiresRenderTarget;
		resourceDesc.AllowDepthStencil = kind == FrameGraphResourceKind::DepthStencil;
		resourceDesc.AllowUnorderedAccess = requiresUnorderedAccess;
		return resourceDesc;
	}

	static RhiOptimizedClearValue BuildTransientOptimizedClearValue(const FrameGraphTextureDesc& desc, FrameGraphResourceKind kind) noexcept
	{
		RhiOptimizedClearValue clearValue{};
		clearValue.Format = desc.format;

		if (kind == FrameGraphResourceKind::DepthStencil)
		{
			clearValue.ValueType = RhiOptimizedClearValue::Type::DepthStencil;
			clearValue.Depth = DepthConvention::GetClearDepth();
			clearValue.Stencil = 0;
			return clearValue;
		}

		clearValue.ValueType = RhiOptimizedClearValue::Type::Color;
		clearValue.Color = desc.clearColor;
		return clearValue;
	}

	static RhiTransientAllocationPool ResolveTransientAllocationPool(FrameGraphResourceKind kind, bool requiresRenderTarget) noexcept
	{
		if (kind == FrameGraphResourceKind::DepthStencil)
		{
			return RhiTransientAllocationPool::DepthStencilTexture;
		}

		if (kind == FrameGraphResourceKind::Buffer)
		{
			return RhiTransientAllocationPool::Buffer;
		}

		return requiresRenderTarget ? RhiTransientAllocationPool::RenderTargetTexture : RhiTransientAllocationPool::Texture;
	}
};

void FrameGraph::BuildTransientMaterializationPlan(FrameGraphPlan& plan) const noexcept
{
	assert(m_renderHardwareInterface != nullptr);

	plan.transients.resources.clear();
	plan.transients.resources.reserve(m_virtualTransientResources.size());

	for (const VirtualTransientResource& transientResource : m_virtualTransientResources)
	{
		const FrameGraphResourceMetadata& resourceMetadata = m_resourceRegistry.GetMetadata(transientResource.handle);
		if (resourceMetadata.ownership != FrameGraphResourceOwnership::Transient)
		{
			continue;
		}

		const bool requiresUnorderedAccess = FrameGraphTransientResourcePlanner::RequiresUnorderedAccess(plan, transientResource.handle);
		const bool requiresRenderTarget = FrameGraphTransientResourcePlanner::RequiresRenderTarget(plan, transientResource.handle);
		const bool isBuffer = resourceMetadata.resourceClass == FrameGraphResourceClass::Buffer;
		const bool hasOptimizedClearValue = !isBuffer &&
		                                         (resourceMetadata.kind == FrameGraphResourceKind::DepthStencil || requiresRenderTarget);
		const RhiBufferResourceDesc bufferResourceDesc =
		    isBuffer ? FrameGraphTransientResourcePlanner::BuildTransientBufferDesc(transientResource.bufferDesc, requiresUnorderedAccess) : RhiBufferResourceDesc{};
		const RhiTextureResourceDesc textureResourceDesc =
		    isBuffer ? RhiTextureResourceDesc{}
		             : FrameGraphTransientResourcePlanner::BuildTransientResourceDesc(
		                   transientResource.textureDesc,
		                   resourceMetadata.kind,
		                   requiresRenderTarget,
		                   requiresUnorderedAccess);
		const RhiResourceAllocationInfo allocationInfo = isBuffer
		                                                     ? m_renderHardwareInterface->GetResourceService().GetBufferAllocationInfo(bufferResourceDesc)
		                                                     : m_renderHardwareInterface->GetResourceService().GetTextureAllocationInfo(textureResourceDesc);
		plan.transients.resources.push_back(
		    FrameGraphTransientResourcePlan{
		        .handle = transientResource.handle,
		        .resourceClass = resourceMetadata.resourceClass,
		        .textureDesc = transientResource.textureDesc,
		        .bufferDesc = transientResource.bufferDesc,
		        .kind = resourceMetadata.kind,
		        .physicalAllocation = FrameGraphTransientResourcePlan::PhysicalAllocationPlan{
		            .physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX,
		            .pool = FrameGraphTransientResourcePlanner::ResolveTransientAllocationPool(resourceMetadata.kind, requiresRenderTarget),
		            .sizeInBytes = allocationInfo.SizeInBytes,
		            .alignment = allocationInfo.Alignment,
		            .memoryBlockOffset = 0,
		            .textureResourceDesc = textureResourceDesc,
		            .bufferResourceDesc = bufferResourceDesc,
		            .optimizedClearValue = hasOptimizedClearValue
		                                       ? FrameGraphTransientResourcePlanner::BuildTransientOptimizedClearValue(transientResource.textureDesc, resourceMetadata.kind)
		                                       : RhiOptimizedClearValue{},
		            .hasOptimizedClearValue = hasOptimizedClearValue,
		            .initialState = resourceMetadata.initialState}});
	}
}

void FrameGraph::EnsureTransientResourcesMaterialized(const FrameGraphPlan& plan) const noexcept
{
	assert(m_transientAllocator != nullptr);
	m_transientAllocator->Prepare(plan.transients);

	for (const FrameGraphTransientResourcePlan& transientPlan : plan.transients.resources)
	{
		const FrameGraphTransientAllocator::AllocationRecord& allocation = m_transientAllocator->Materialize(transientPlan);
		FrameGraphResourceAccess access{};

		switch (transientPlan.kind)
		{
			case FrameGraphResourceKind::DepthStencil:
				access.resource = allocation.resource;
				access.depthStencilView = allocation.depthStencilView;
				if (allocation.shaderResourceView)
				{
					access.shaderResourceView = allocation.shaderResourceView;
				}
				break;
			case FrameGraphResourceKind::ColorRenderTarget:
				access.resource = allocation.resource;
				access.renderTargetView = allocation.renderTargetView;
				if (allocation.shaderResourceView)
				{
					access.shaderResourceView = allocation.shaderResourceView;
				}
				if (allocation.unorderedAccessView)
				{
					access.unorderedAccessView = allocation.unorderedAccessView;
				}
				break;
			case FrameGraphResourceKind::Buffer:
				access.resource = allocation.resource;
				if (allocation.shaderResourceView)
				{
					access.shaderResourceView = allocation.shaderResourceView;
				}
				if (allocation.unorderedAccessView)
				{
					access.unorderedAccessView = allocation.unorderedAccessView;
				}
				break;
			default:
				assert(false);
				break;
		}

		m_resourceResolver.SetResolvedAccess(transientPlan.handle, access);
	}
}
