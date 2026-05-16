#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "Config/DepthConvention.h"

#include <cassert>
#include <string>

namespace
{
	bool RequiresUnorderedAccess(const FrameGraphPlan& plan, FrameGraphResourceHandle handle) noexcept
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

	RhiBufferResourceDesc BuildTransientBufferDesc(const FrameGraphBufferDesc& desc, bool requiresUnorderedAccess) noexcept
	{
		return RhiBufferResourceDesc{
		    .SizeInBytes = desc.sizeInBytes,
		    .StrideInBytes = desc.strideInBytes,
		    .AllowUnorderedAccess = requiresUnorderedAccess};
	}

	RhiTextureResourceDesc BuildTransientResourceDesc(
	    const FrameGraphTextureDesc& desc,
	    FrameGraphResourceKind kind,
	    bool requiresUnorderedAccess) noexcept
	{
		RhiTextureResourceDesc resourceDesc{};
		resourceDesc.Width = desc.width;
		resourceDesc.Height = desc.height;
		resourceDesc.Format = desc.format;
		resourceDesc.MipLevels = 1;
		resourceDesc.AllowRenderTarget = kind == FrameGraphResourceKind::ColorRenderTarget;
		resourceDesc.AllowDepthStencil = kind == FrameGraphResourceKind::DepthStencil;
		resourceDesc.AllowUnorderedAccess = requiresUnorderedAccess;
		return resourceDesc;
	}

	RhiOptimizedClearValue BuildTransientOptimizedClearValue(const FrameGraphTextureDesc& desc, FrameGraphResourceKind kind) noexcept
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
		clearValue.Color = {0.0f, 0.0f, 0.0f, 1.0f};
		return clearValue;
	}

	FrameGraphTransientResourcePlan::AllocationPool ResolveTransientAllocationPool(FrameGraphResourceKind kind) noexcept
	{
		if (kind == FrameGraphResourceKind::DepthStencil)
		{
			return FrameGraphTransientResourcePlan::AllocationPool::Depth;
		}

		if (kind == FrameGraphResourceKind::Buffer)
		{
			return FrameGraphTransientResourcePlan::AllocationPool::Buffer;
		}

		return FrameGraphTransientResourcePlan::AllocationPool::Color;
	}
}  // namespace

void FrameGraph::BuildTransientMaterializationPlan(FrameGraphPlan& plan) const noexcept
{
	assert(m_renderHardwareInterface != nullptr);

	plan.transientResources.clear();
	plan.transientResources.reserve(m_virtualTransientResources.size());

	for (const VirtualTransientResource& transientResource : m_virtualTransientResources)
	{
		const FrameGraphResourceMetadata& resourceMetadata = m_resourceRegistry.GetMetadata(transientResource.handle);
		if (resourceMetadata.ownership != FrameGraphResourceOwnership::Transient)
		{
			continue;
		}

		const bool requiresUnorderedAccess = RequiresUnorderedAccess(plan, transientResource.handle);
		const bool isBuffer = resourceMetadata.resourceClass == FrameGraphResourceClass::Buffer;
		const RhiBufferResourceDesc bufferResourceDesc =
		    isBuffer ? BuildTransientBufferDesc(transientResource.bufferDesc, requiresUnorderedAccess) : RhiBufferResourceDesc{};
		const RhiTextureResourceDesc textureResourceDesc =
		    isBuffer ? RhiTextureResourceDesc{}
		             : BuildTransientResourceDesc(transientResource.textureDesc, resourceMetadata.kind, requiresUnorderedAccess);
		const RhiResourceAllocationInfo allocationInfo = isBuffer
		                                                     ? m_renderHardwareInterface->GetBufferAllocationInfo(bufferResourceDesc)
		                                                     : m_renderHardwareInterface->GetTextureAllocationInfo(textureResourceDesc);
		const std::uint32_t allocationIndex = static_cast<std::uint32_t>(plan.transientResources.size());
		plan.transientResources.push_back(
		    FrameGraphTransientResourcePlan{
		        .handle = transientResource.handle,
		        .resourceClass = resourceMetadata.resourceClass,
		        .textureDesc = transientResource.textureDesc,
		        .bufferDesc = transientResource.bufferDesc,
		        .kind = resourceMetadata.kind,
		        .physicalAllocation = FrameGraphTransientResourcePlan::PhysicalAllocationPlan{
		            .allocationIndex = allocationIndex,
		            .physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX,
		            .pool = ResolveTransientAllocationPool(resourceMetadata.kind),
		            .sizeInBytes = allocationInfo.SizeInBytes,
		            .alignment = allocationInfo.Alignment,
		            .heapOffset = 0,
		            .textureResourceDesc = textureResourceDesc,
		            .bufferResourceDesc = bufferResourceDesc,
		            .optimizedClearValue = isBuffer
		                                       ? RhiOptimizedClearValue{}
		                                       : BuildTransientOptimizedClearValue(transientResource.textureDesc, resourceMetadata.kind),
		            .hasOptimizedClearValue = !isBuffer,
		            .initialState = resourceMetadata.initialState}});
	}
}

void FrameGraph::EnsureTransientResourcesMaterialized(const FrameGraphPlan& plan) const noexcept
{
	assert(m_transientAllocator != nullptr);

	for (const FrameGraphTransientResourcePlan& transientPlan : plan.transientResources)
	{
		(void) m_transientAllocator->Materialize(transientPlan);
	}
}
