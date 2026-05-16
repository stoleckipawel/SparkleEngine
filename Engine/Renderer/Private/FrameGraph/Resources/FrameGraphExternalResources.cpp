#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Commands/RenderCommandContext.h"

#include <cassert>

static const auto g_frameGraphExternalLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

namespace
{
	bool RequiresUnorderedAccessView(const FrameGraphPlan& plan, FrameGraphResourceHandle handle) noexcept
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
}

void FrameGraph::SyncImportedResourceAccesses() const noexcept
{
	assert(m_renderHardwareInterface != nullptr);

	for (const FrameGraphResourceHandle handle : m_resourceRegistry.GetRegisteredHandles())
	{
		const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
		FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
		if (metadata.ownership != FrameGraphResourceOwnership::Imported || metadata.kind == FrameGraphResourceKind::BackBuffer)
		{
			continue;
		}

		if (!access.resource)
		{
			continue;
		}

		if (metadata.kind == FrameGraphResourceKind::ColorRenderTarget)
		{
			if (!access.renderTargetView)
			{
				access.renderTargetView = m_renderHardwareInterface->CreateResourceView(
				    RhiResourceViewDesc::RenderTarget(access.resource, metadata.textureDesc.format));
			}

			if (!access.shaderResourceView)
			{
				access.shaderResourceView = m_renderHardwareInterface->CreateResourceView(
				    RhiResourceViewDesc::TextureShaderResource(access.resource, metadata.textureDesc.format));
			}

			if (RequiresUnorderedAccessView(m_compiledPlan, handle))
			{
				if (!m_renderHardwareInterface->SupportsUnorderedAccess(access.resource))
				{
					SPDLOG_LOGGER_WARN(
					    g_frameGraphExternalLogger,
					    "FrameGraph::SyncImportedResourceAccesses: imported texture is missing unordered-access support.");
					assert(false);
				}

				if (!access.unorderedAccessView)
				{
					access.unorderedAccessView = m_renderHardwareInterface->CreateResourceView(
					    RhiResourceViewDesc::TextureUnorderedAccess(access.resource, metadata.textureDesc.format));
				}
			}
		}
		else if (metadata.kind == FrameGraphResourceKind::DepthStencil)
		{
			if (!access.depthStencilView)
			{
				access.depthStencilView = m_renderHardwareInterface->CreateResourceView(
				    RhiResourceViewDesc::DepthStencil(access.resource, metadata.textureDesc.format));
			}
		}
		else if (metadata.kind == FrameGraphResourceKind::Buffer)
		{
			if (!access.shaderResourceView)
			{
				access.shaderResourceView = m_renderHardwareInterface->CreateResourceView(RhiResourceViewDesc::BufferShaderResource(
				    access.resource,
				    metadata.bufferDesc.sizeInBytes,
				    metadata.bufferDesc.strideInBytes));
			}

			if (RequiresUnorderedAccessView(m_compiledPlan, handle))
			{
				if (!m_renderHardwareInterface->SupportsUnorderedAccess(access.resource))
				{
					SPDLOG_LOGGER_WARN(
					    g_frameGraphExternalLogger,
					    "FrameGraph::SyncImportedResourceAccesses: imported buffer is missing unordered-access support.");
					assert(false);
				}

				if (!access.unorderedAccessView)
				{
					access.unorderedAccessView = m_renderHardwareInterface->CreateResourceView(RhiResourceViewDesc::BufferUnorderedAccess(
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
		if (metadata.ownership != FrameGraphResourceOwnership::Imported || metadata.kind == FrameGraphResourceKind::BackBuffer)
		{
			continue;
		}

		FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
		if (access.renderTargetView)
		{
			m_renderHardwareInterface->ReleaseResourceView(access.renderTargetView);
			access.renderTargetView = {};
		}

		if (access.depthStencilView)
		{
			m_renderHardwareInterface->ReleaseResourceView(access.depthStencilView);
			access.depthStencilView = {};
		}

		if (access.shaderResourceView)
		{
			m_renderHardwareInterface->ReleaseResourceView(access.shaderResourceView);
			access.shaderResourceView = {};
		}

		if (access.unorderedAccessView)
		{
			m_renderHardwareInterface->ReleaseResourceView(access.unorderedAccessView);
			access.unorderedAccessView = {};
		}
	}
}
