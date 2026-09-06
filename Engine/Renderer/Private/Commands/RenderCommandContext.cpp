#include "PCH.h"
#include "Commands/RenderCommandContext.h"

#include <cstdio>

static const auto g_renderCommandContextLogger = Logging::GetOrCreateLogger("Renderer.CommandContext");

RenderCommandContext::RenderCommandContext(RenderCommandList& commandList) noexcept :
    m_commandList(&commandList)
{
}

void RenderCommandContext::EnableDrawDispatchDiagnostics() noexcept
{
	m_drawDispatchDiagnosticsEnabled = SupportsDiagnosticScopes();
}

void RenderCommandContext::SetPipeline(const RenderPipeline& pipeline) noexcept
{
	m_commandList->SetPipeline(pipeline);
}

void RenderCommandContext::SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_commandList->SetGraphicsBindingLayout(bindingLayout);
}

void RenderCommandContext::SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_commandList->SetComputeBindingLayout(bindingLayout);
}

void RenderCommandContext::SetRayTracingBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_commandList->SetRayTracingBindingLayout(bindingLayout);
}

void RenderCommandContext::SetRayTracingPipeline(const RayTracingPipeline& pipeline) noexcept
{
	m_commandList->SetRayTracingPipeline(pipeline);
}

void RenderCommandContext::SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept
{
	m_commandList->SetPrimitiveTopology(topology);
}

void RenderCommandContext::BindVertexBuffer(const RhiVertexBufferView& view) noexcept
{
	m_commandList->BindVertexBuffer(view);
}

void RenderCommandContext::BindIndexBuffer(const RhiIndexBufferView& view) noexcept
{
	m_commandList->BindIndexBuffer(view);
}

void RenderCommandContext::BindConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsConstantBuffer(bindingIndex, gpuAddress);
}

void RenderCommandContext::SetPushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_commandList->SetGraphicsPushConstants(bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void RenderCommandContext::BindShaderResourceAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsShaderResource(bindingIndex, gpuAddress);
}

void RenderCommandContext::BindUnorderedAccessAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsUnorderedAccess(bindingIndex, gpuAddress);
}

void RenderCommandContext::BindAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	m_commandList->BindGraphicsAccelerationStructure(bindingIndex, resource);
}

void RenderCommandContext::BindDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_commandList->BindGraphicsDescriptorTable(bindingIndex, baseDescriptor);
}

void RenderCommandContext::BindDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	m_commandList->BindGraphicsDescriptorTable(bindingIndex, tableBinding);
}

void RenderCommandContext::BindComputeConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeConstantBuffer(bindingIndex, gpuAddress);
}

void RenderCommandContext::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_commandList->BindComputeDescriptorTable(bindingIndex, baseDescriptor);
}

void RenderCommandContext::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	m_commandList->BindComputeDescriptorTable(bindingIndex, tableBinding);
}

void RenderCommandContext::SetComputePushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_commandList->SetComputePushConstants(bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void RenderCommandContext::BindComputeShaderResourceAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeShaderResource(bindingIndex, gpuAddress);
}

void RenderCommandContext::BindComputeUnorderedAccessAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeUnorderedAccess(bindingIndex, gpuAddress);
}

void RenderCommandContext::BindComputeAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	m_commandList->BindComputeAccelerationStructure(bindingIndex, resource);
}

void RenderCommandContext::BindRayTracingConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindRayTracingConstantBuffer(bindingIndex, gpuAddress);
}

void RenderCommandContext::BindRayTracingDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_commandList->BindRayTracingDescriptorTable(bindingIndex, baseDescriptor);
}

void RenderCommandContext::BindRayTracingDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	m_commandList->BindRayTracingDescriptorTable(bindingIndex, tableBinding);
}

void RenderCommandContext::SetRayTracingPushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_commandList->SetRayTracingPushConstants(bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void RenderCommandContext::BindRayTracingShaderResourceAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindRayTracingShaderResource(bindingIndex, gpuAddress);
}

void RenderCommandContext::BindRayTracingUnorderedAccessAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindRayTracingUnorderedAccess(bindingIndex, gpuAddress);
}

void RenderCommandContext::BindRayTracingAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	m_commandList->BindRayTracingAccelerationStructure(bindingIndex, resource);
}

void RenderCommandContext::SetRenderTarget(RhiCpuDescriptorHandle renderTarget, const RhiCpuDescriptorHandle* depthStencil) noexcept
{
	m_commandList->SetRenderTarget(renderTarget, depthStencil);
}

void RenderCommandContext::SetRenderTargets(
    std::uint32_t renderTargetCount,
    const RhiCpuDescriptorHandle* renderTargets,
    const RhiCpuDescriptorHandle* depthStencil) noexcept
{
	m_commandList->SetRenderTargets(renderTargetCount, renderTargets, depthStencil);
}

void RenderCommandContext::ClearRenderTarget(RhiCpuDescriptorHandle renderTarget, RhiClearColorView color) noexcept
{
	m_commandList->ClearRenderTarget(renderTarget, color);
}

void RenderCommandContext::ClearDepthStencil(RhiCpuDescriptorHandle depthStencil, float depth, std::uint8_t stencil) noexcept
{
	m_commandList->ClearDepthStencil(depthStencil, depth, stencil);
}

void RenderCommandContext::EndRasterPass() noexcept
{
	m_commandList->EndRasterPass();
}

void RenderCommandContext::SetViewport(const RhiViewport& viewport) noexcept
{
	m_commandList->SetViewport(viewport);
}

void RenderCommandContext::SetViewport(float xPosition, float yPosition, float width, float height, float minDepth, float maxDepth) noexcept
{
	SetViewport(RhiViewport{.X = xPosition, .Y = yPosition, .Width = width, .Height = height, .MinDepth = minDepth, .MaxDepth = maxDepth});
}

void RenderCommandContext::SetScissorRect(const RhiRect& scissorRect) noexcept
{
	m_commandList->SetScissorRect(scissorRect);
}

void RenderCommandContext::SetScissorRect(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom) noexcept
{
	SetScissorRect(RhiRect{.Left = left, .Top = top, .Right = right, .Bottom = bottom});
}

void RenderCommandContext::EmitDrawMarker() noexcept
{
	char label[32];
	std::snprintf(label, sizeof(label), "Draw.%u", m_drawCount++);
	m_commandList->InsertDiagnosticMarker(label, {});
}

void RenderCommandContext::EmitDispatchMarker(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	char label[64];
	std::snprintf(label, sizeof(label), "Dispatch.%u.%ux%ux%u", m_dispatchCount++, groupCountX, groupCountY, groupCountZ);
	m_commandList->InsertDiagnosticMarker(label, {});
}

void RenderCommandContext::DrawIndexedInstanced(
    std::uint32_t indexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	if (m_drawDispatchDiagnosticsEnabled)
	{
		EmitDrawMarker();
	}
	m_commandList
	    ->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void RenderCommandContext::DrawInstanced(
    std::uint32_t vertexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	if (m_drawDispatchDiagnosticsEnabled)
	{
		EmitDrawMarker();
	}
	m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void RenderCommandContext::Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	if (m_drawDispatchDiagnosticsEnabled)
	{
		EmitDispatchMarker(groupCountX, groupCountY, groupCountZ);
	}
	m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void RenderCommandContext::TraceRays(const TraceRaysDesc& desc) noexcept
{
	if (m_drawDispatchDiagnosticsEnabled)
	{
		EmitDispatchMarker(desc.Width, desc.Height, desc.Depth);
	}
	m_commandList->TraceRays(desc);
}

void RenderCommandContext::BuildBottomLevelAccelerationStructure(
    const RhiRayTracingGeometryDesc& geometry,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->BuildBottomLevelAccelerationStructure(geometry, scratchGpuAddress, resultGpuAddress);
		return;
	}
	Diagnostics::Fatal(g_renderCommandContextLogger, __FILE__, __LINE__, "BLAS build has no active render command list.");
}

void RenderCommandContext::BuildTopLevelAccelerationStructure(
    RhiGpuVirtualAddress instanceDescsGpuAddress,
    std::uint32_t instanceCount,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress,
    ERhiClassicTlasBuildMode buildMode) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList
		    ->BuildTopLevelAccelerationStructure(instanceDescsGpuAddress, instanceCount, scratchGpuAddress, resultGpuAddress, buildMode);
		return;
	}
	Diagnostics::Fatal(g_renderCommandContextLogger, __FILE__, __LINE__, "Classic TLAS build has no active render command list.");
}

void RenderCommandContext::BuildPartitionedTopLevelAccelerationStructure(const RhiPartitionedTlasBuildCommandDesc& desc) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->BuildPartitionedTopLevelAccelerationStructure(desc);
		return;
	}
	Diagnostics::Fatal(g_renderCommandContextLogger, __FILE__, __LINE__, "Partitioned TLAS build has no active render command list.");
}

void RenderCommandContext::TrackResource(RhiResourceHandle resource) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->TrackResource(resource);
	}
}

bool RenderCommandContext::SupportsDiagnosticScopes() const noexcept
{
	return m_commandList->SupportsDiagnosticScopes();
}

void RenderCommandContext::BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	m_commandList->BeginDiagnosticScope(label, color);
}

void RenderCommandContext::EndDiagnosticScope() noexcept
{
	m_commandList->EndDiagnosticScope();
}

void RenderCommandContext::InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	m_commandList->InsertDiagnosticMarker(label, color);
}

std::uint16_t RenderCommandContext::AcquireGpuDiagnosticScopeDepth() noexcept
{
	return m_gpuDiagnosticScopeDepth++;
}

void RenderCommandContext::ReleaseGpuDiagnosticScopeDepth() noexcept
{
	if (m_gpuDiagnosticScopeDepth == 0)
	{
		Diagnostics::Fatal(g_renderCommandContextLogger, __FILE__, __LINE__, "GPU diagnostic scope depth underflowed.");
	}
	--m_gpuDiagnosticScopeDepth;
}

void RenderCommandContext::CopyResource(RhiResourceHandle destinationResource, RhiResourceHandle sourceResource) noexcept
{
	m_commandList->CopyResource(destinationResource, sourceResource);
}

void RenderCommandContext::AliasResource(RhiResourceHandle beforeResource, RhiResourceHandle afterResource) noexcept
{
	m_commandList->AliasResource(beforeResource, afterResource);
}

void RenderCommandContext::TransitionResource(RhiResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	m_commandList->TransitionResource(resource, before, after);
}

void RenderCommandContext::UnorderedAccessBarrier(RhiResourceHandle resource) noexcept
{
	m_commandList->UnorderedAccessBarrier(resource);
}
