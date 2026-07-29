#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cassert>
#include <format>

static const auto g_frameGraphExternalLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

class FrameGraphExternalResourceContract final
{
  public:
	static std::string FormatResourceName(const FrameGraphResourceMetadata& metadata)
	{
		return metadata.debugName.empty() ? std::format("handle {}", metadata.handle.index) : metadata.debugName;
	}

	static void FailMissingUnorderedAccessSupport(const FrameGraphResourceMetadata& metadata) noexcept
	{
		Diagnostics::Fatal(
		    g_frameGraphExternalLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "FrameGraph external resource validation failed: resource='{}' handle={} ownership={} requiredUsage=UnorderedAccess "
		        "remediation='create/import the resource with unordered-access support or remove UAV declarations for this pass path'",
		        FormatResourceName(metadata),
		        metadata.handle.index,
		        IsExternalFrameGraphResource(metadata.ownership) ? "External" : "Internal"));
	}

	static bool RequiresUsage(const FrameGraphPlan& plan, FrameGraphResourceHandle handle, ResourceUsage usage) noexcept
	{
		for (const FrameGraphPassNode& passRecord : plan.passes)
		{
			for (const PassResourceDeclaration& declaration : passRecord.declarations)
			{
				if (declaration.handle == handle && declaration.usage == usage)
				{
					return true;
				}
			}
		}

		return false;
	}

	static bool RequiresUnorderedAccessView(const FrameGraphPlan& plan, FrameGraphResourceHandle handle) noexcept
	{
		return RequiresUsage(plan, handle, ResourceUsage::UnorderedAccess);
	}
};

void FrameGraph::SyncImportedResourceAccesses() const noexcept
{
	assert(m_renderHardwareInterface != nullptr);

	for (const FrameGraphResourceHandle handle : m_resourceRegistry.GetRegisteredHandles())
	{
		const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
		FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
		if (!IsExternalFrameGraphResource(metadata.ownership) || metadata.kind == FrameGraphResourceKind::BackBuffer)
		{
			continue;
		}

		if (!access.resource)
		{
			continue;
		}

		if (metadata.kind == FrameGraphResourceKind::AccelerationStructure)
		{
			continue;
		}

		if (metadata.kind == FrameGraphResourceKind::ColorRenderTarget)
		{
			if (FrameGraphExternalResourceContract::RequiresUsage(m_compiledPlan, handle, ResourceUsage::RenderTarget) && !access.renderTargetView)
			{
				access.renderTargetView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
				    RhiResourceViewDesc::RenderTarget(access.resource, metadata.textureDesc.format));
			}

			if (FrameGraphExternalResourceContract::RequiresUsage(m_compiledPlan, handle, ResourceUsage::ShaderRead) && !access.shaderResourceView)
			{
				access.shaderResourceView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
				    RhiResourceViewDesc::TextureShaderResource(access.resource, metadata.textureDesc.format));
				access.ownsShaderResourceView = static_cast<bool>(access.shaderResourceView);
			}

			if (FrameGraphExternalResourceContract::RequiresUnorderedAccessView(m_compiledPlan, handle))
			{
				if (!m_renderHardwareInterface->GetResourceService().SupportsUnorderedAccess(access.resource))
				{
					FrameGraphExternalResourceContract::FailMissingUnorderedAccessSupport(metadata);
				}

				if (!access.unorderedAccessView)
				{
					access.unorderedAccessView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
					    RhiResourceViewDesc::TextureUnorderedAccess(access.resource, metadata.textureDesc.format));
				}
			}
		}
		else if (metadata.kind == FrameGraphResourceKind::DepthStencil)
		{
			if (!access.depthStencilView)
			{
				access.depthStencilView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
				    RhiResourceViewDesc::DepthStencil(access.resource, metadata.textureDesc.format));
			}

			if (FrameGraphExternalResourceContract::RequiresUsage(m_compiledPlan, handle, ResourceUsage::ShaderRead) && !access.shaderResourceView)
			{
				access.shaderResourceView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
				    RhiResourceViewDesc::TextureShaderResource(access.resource, metadata.textureDesc.format));
				access.ownsShaderResourceView = static_cast<bool>(access.shaderResourceView);
			}
		}
		else if (metadata.kind == FrameGraphResourceKind::Buffer)
		{
			if (FrameGraphExternalResourceContract::RequiresUsage(m_compiledPlan, handle, ResourceUsage::ShaderRead) && !access.shaderResourceView)
			{
				access.shaderResourceView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
				    RhiResourceViewDesc::BufferShaderResource(
				        access.resource,
				        metadata.bufferDesc.sizeInBytes,
				        metadata.bufferDesc.strideInBytes));
				access.ownsShaderResourceView = static_cast<bool>(access.shaderResourceView);
			}

			if (FrameGraphExternalResourceContract::RequiresUnorderedAccessView(m_compiledPlan, handle))
			{
				if (!m_renderHardwareInterface->GetResourceService().SupportsUnorderedAccess(access.resource))
				{
					FrameGraphExternalResourceContract::FailMissingUnorderedAccessSupport(metadata);
				}

				if (!access.unorderedAccessView)
				{
					access.unorderedAccessView = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
					    RhiResourceViewDesc::BufferUnorderedAccess(
					        access.resource,
					        metadata.bufferDesc.sizeInBytes,
					        metadata.bufferDesc.strideInBytes));
				}
			}
		}
	}
}

void FrameGraph::ReleaseExternalResourceViews() noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	for (const FrameGraphResourceHandle handle : m_resourceRegistry.GetRegisteredHandles())
	{
		const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
		if (!IsExternalFrameGraphResource(metadata.ownership) || metadata.kind == FrameGraphResourceKind::BackBuffer)
		{
			continue;
		}

		ReleaseExternalResourceViews(handle);
	}
}

void FrameGraph::ReleaseExternalResourceViews(FrameGraphResourceHandle handle) noexcept
{
	if (m_renderHardwareInterface == nullptr || !handle.IsValid() || !m_resourceRegistry.IsRegistered(handle))
	{
		return;
	}

	FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	if (access.renderTargetView)
	{
		m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(access.renderTargetView);
		access.renderTargetView = {};
	}

	if (access.depthStencilView)
	{
		m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(access.depthStencilView);
		access.depthStencilView = {};
	}

	if (access.shaderResourceView)
	{
		if (access.ownsShaderResourceView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(access.shaderResourceView);
		}
		access.shaderResourceView = {};
		access.ownsShaderResourceView = false;
	}

	if (access.unorderedAccessView)
	{
		m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(access.unorderedAccessView);
		access.unorderedAccessView = {};
	}
}
