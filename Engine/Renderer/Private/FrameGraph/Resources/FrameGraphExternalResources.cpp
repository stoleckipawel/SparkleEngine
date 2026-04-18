#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "GPU/CommandContext.h"

#include <cassert>

static const auto g_frameGraphExternalLogger = Engine::Logging::GetOrCreateLogger("Renderer.FrameGraph");

namespace
{
	bool RequiresUnorderedAccessView(const FrameGraph::CompiledPlan& plan, ResourceHandle handle) noexcept
	{
		for (const FrameGraph::CompilePassRecord& passRecord : plan.passes)
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

	for (const ResourceHandle handle : m_resourceRegistry.GetRegisteredHandles())
	{
		FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
		FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
		if (metadata.ownership != FrameGraphResourceOwnership::Imported || metadata.kind == FrameGraphResourceKind::BackBuffer)
		{
			continue;
		}

		if (!access.externalResource)
		{
			continue;
		}

		if (metadata.kind == FrameGraphResourceKind::ColorRenderTarget)
		{
			if (!access.renderTargetView)
			{
				access.renderTargetView = m_renderHardwareInterface->AllocateDescriptor(RhiDescriptorHeapType::RenderTarget).CpuHandle;
			}

			m_renderHardwareInterface->CreateRenderTargetView(
			    access.externalResource,
			    metadata.textureDesc.format,
			    access.renderTargetView);

			if (!access.shaderResourceViewCpu || !access.shaderResourceViewGpu)
			{
				const RhiDescriptorAllocation allocation =
				    m_renderHardwareInterface->AllocateDescriptor(RhiDescriptorHeapType::ShaderResource);
				access.shaderResourceViewCpu = allocation.CpuHandle;
				access.shaderResourceViewGpu = allocation.GpuHandle;
			}

			m_renderHardwareInterface->CreateTextureShaderResourceView(
			    access.externalResource,
			    metadata.textureDesc.format,
			    access.shaderResourceViewCpu);

			if (RequiresUnorderedAccessView(m_compiledPlan, handle))
			{
				if (!m_renderHardwareInterface->SupportsUnorderedAccess(access.externalResource))
				{
					SPDLOG_LOGGER_WARN(
					    g_frameGraphExternalLogger,
					    "FrameGraph::SyncImportedResourceAccesses: imported texture is missing unordered-access support.");
					assert(false);
				}

				if (!access.unorderedAccessViewCpu || !access.unorderedAccessViewGpu)
				{
					const RhiDescriptorAllocation allocation =
					    m_renderHardwareInterface->AllocateDescriptor(RhiDescriptorHeapType::ShaderResource);
					access.unorderedAccessViewCpu = allocation.CpuHandle;
					access.unorderedAccessViewGpu = allocation.GpuHandle;
				}

				m_renderHardwareInterface->CreateTextureUnorderedAccessView(
				    access.externalResource,
				    metadata.textureDesc.format,
				    access.unorderedAccessViewCpu);
			}
		}
		else if (metadata.kind == FrameGraphResourceKind::DepthStencil)
		{
			if (!access.depthStencilView)
			{
				access.depthStencilView = m_renderHardwareInterface->AllocateDescriptor(RhiDescriptorHeapType::DepthStencil).CpuHandle;
			}

			m_renderHardwareInterface->CreateDepthStencilView(
			    access.externalResource,
			    metadata.textureDesc.format,
			    access.depthStencilView);
		}
		else if (metadata.kind == FrameGraphResourceKind::Buffer)
		{
			if (!access.shaderResourceViewCpu || !access.shaderResourceViewGpu)
			{
				const RhiDescriptorAllocation allocation =
				    m_renderHardwareInterface->AllocateDescriptor(RhiDescriptorHeapType::ShaderResource);
				access.shaderResourceViewCpu = allocation.CpuHandle;
				access.shaderResourceViewGpu = allocation.GpuHandle;
			}

			m_renderHardwareInterface->CreateBufferShaderResourceView(
			    access.externalResource,
			    metadata.bufferDesc.sizeInBytes,
			    metadata.bufferDesc.strideInBytes,
			    access.shaderResourceViewCpu);

			if (RequiresUnorderedAccessView(m_compiledPlan, handle))
			{
				if (!m_renderHardwareInterface->SupportsUnorderedAccess(access.externalResource))
				{
					SPDLOG_LOGGER_WARN(
					    g_frameGraphExternalLogger,
					    "FrameGraph::SyncImportedResourceAccesses: imported buffer is missing unordered-access support.");
					assert(false);
				}

				if (!access.unorderedAccessViewCpu || !access.unorderedAccessViewGpu)
				{
					const RhiDescriptorAllocation allocation =
					    m_renderHardwareInterface->AllocateDescriptor(RhiDescriptorHeapType::ShaderResource);
					access.unorderedAccessViewCpu = allocation.CpuHandle;
					access.unorderedAccessViewGpu = allocation.GpuHandle;
				}

				m_renderHardwareInterface->CreateBufferUnorderedAccessView(
				    access.externalResource,
				    metadata.bufferDesc.sizeInBytes,
				    metadata.bufferDesc.strideInBytes,
				    access.unorderedAccessViewCpu);
			}
		}
	}
}

void FrameGraph::ReleaseExternalViewDescriptors() noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	for (const ResourceHandle handle : m_resourceRegistry.GetRegisteredHandles())
	{
		FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
		if (access.renderTargetView)
		{
			m_renderHardwareInterface->ReleaseDescriptor(
			    RhiDescriptorHeapType::RenderTarget,
			    RhiDescriptorAllocation{.CpuHandle = access.renderTargetView});
			access.renderTargetView = {};
		}

		if (access.depthStencilView)
		{
			m_renderHardwareInterface->ReleaseDescriptor(
			    RhiDescriptorHeapType::DepthStencil,
			    RhiDescriptorAllocation{.CpuHandle = access.depthStencilView});
			access.depthStencilView = {};
		}

		if (access.shaderResourceViewCpu || access.shaderResourceViewGpu)
		{
			m_renderHardwareInterface->ReleaseDescriptor(
			    RhiDescriptorHeapType::ShaderResource,
			    RhiDescriptorAllocation{.CpuHandle = access.shaderResourceViewCpu, .GpuHandle = access.shaderResourceViewGpu});
			access.shaderResourceViewCpu = {};
			access.shaderResourceViewGpu = {};
		}

		if (access.unorderedAccessViewCpu || access.unorderedAccessViewGpu)
		{
			m_renderHardwareInterface->ReleaseDescriptor(
			    RhiDescriptorHeapType::ShaderResource,
			    RhiDescriptorAllocation{.CpuHandle = access.unorderedAccessViewCpu, .GpuHandle = access.unorderedAccessViewGpu});
			access.unorderedAccessViewCpu = {};
			access.unorderedAccessViewGpu = {};
		}
	}
}