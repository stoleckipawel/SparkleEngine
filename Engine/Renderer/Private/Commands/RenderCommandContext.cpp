#include "PCH.h"
#include "Commands/RenderCommandContext.h"

#include "Core/Public/Diagnostics/LiveProfiler.h"

#include <cstdio>

RenderCommandContext::RenderCommandContext(RenderCommandList& commandList) noexcept : m_commandList(&commandList) {}

void RenderCommandContext::EnableDrawDispatchDiagnostics() noexcept
{
	m_drawDispatchDiagnosticsEnabled = SupportsDiagnosticScopes();
}

void RenderCommandContext::SetPipelineState(const RenderPipelineState& pipelineState) noexcept
{
	m_commandList->SetPipelineState(pipelineState);
}

void RenderCommandContext::SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_commandList->SetGraphicsBindingLayout(bindingLayout);
}

void RenderCommandContext::SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_commandList->SetComputeBindingLayout(bindingLayout);
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

void RenderCommandContext::BindConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsConstantBuffer(rootParameterIndex, gpuAddress);
}

void RenderCommandContext::SetRoot32BitConstants(
    std::uint32_t rootParameterIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_commandList->SetGraphicsRootConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void RenderCommandContext::BindRootShaderResourceView(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsShaderResource(rootParameterIndex, gpuAddress);
}

void RenderCommandContext::BindRootUnorderedAccessView(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsUnorderedAccess(rootParameterIndex, gpuAddress);
}

void RenderCommandContext::BindDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_commandList->BindGraphicsDescriptorTable(rootParameterIndex, baseDescriptor);
}

void RenderCommandContext::BindDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	m_commandList->BindGraphicsDescriptorTable(rootParameterIndex, tableBinding);
}

void RenderCommandContext::BindComputeRootConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeConstantBuffer(rootParameterIndex, gpuAddress);
}

void RenderCommandContext::BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_commandList->BindComputeDescriptorTable(rootParameterIndex, baseDescriptor);
}

void RenderCommandContext::BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	m_commandList->BindComputeDescriptorTable(rootParameterIndex, tableBinding);
}

void RenderCommandContext::SetComputeRoot32BitConstants(
    std::uint32_t rootParameterIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_commandList->SetComputeRootConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void RenderCommandContext::BindComputeRootShaderResourceView(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeShaderResource(rootParameterIndex, gpuAddress);
}

void RenderCommandContext::BindComputeRootUnorderedAccessView(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeUnorderedAccess(rootParameterIndex, gpuAddress);
}

void RenderCommandContext::SetDescriptorHeaps(std::uint32_t heapCount, const NativeDescriptorHeapHandle* heaps) noexcept
{
	m_commandList->SetDescriptorHeaps(heapCount, heaps);
}

void RenderCommandContext::SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept
{
	m_commandList->SetRenderTarget(rtv, dsv);
}

void RenderCommandContext::SetRenderTargets(std::uint32_t numRTVs, const RhiCpuDescriptorHandle* rtvs, const RhiCpuDescriptorHandle* dsv) noexcept
{
	m_commandList->SetRenderTargets(numRTVs, rtvs, dsv);
}

void RenderCommandContext::ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept
{
	m_commandList->ClearRenderTarget(rtv, color);
}

void RenderCommandContext::ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil) noexcept
{
	m_commandList->ClearDepthStencil(dsv, depth, stencil);
}

void RenderCommandContext::SetViewport(const RhiViewport& viewport) noexcept
{
	m_commandList->SetViewport(viewport);
}

void RenderCommandContext::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) noexcept
{
	SetViewport(RhiViewport{.X = x, .Y = y, .Width = width, .Height = height, .MinDepth = minDepth, .MaxDepth = maxDepth});
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
	Diagnostics::LiveProfiler::Get().AccumulateDrawCall(indexCountPerInstance, instanceCount, /*indexed*/ true);
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
	Diagnostics::LiveProfiler::Get().AccumulateDrawCall(vertexCountPerInstance, instanceCount, /*indexed*/ false);
	m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void RenderCommandContext::Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	if (m_drawDispatchDiagnosticsEnabled)
	{
		EmitDispatchMarker(groupCountX, groupCountY, groupCountZ);
	}
	Diagnostics::LiveProfiler::Get().AccumulateDispatch(groupCountX, groupCountY, groupCountZ);
	m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void RenderCommandContext::BuildBottomLevelAccelerationStructure(
    const RhiRayTracingGeometryDesc& geometry,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->BuildBottomLevelAccelerationStructure(geometry, scratchGpuAddress, resultGpuAddress);
	}
}

void RenderCommandContext::BuildTopLevelAccelerationStructure(
    RhiGpuVirtualAddress instanceDescsGpuAddress,
    std::uint32_t instanceCount,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->BuildTopLevelAccelerationStructure(instanceDescsGpuAddress, instanceCount, scratchGpuAddress, resultGpuAddress);
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

void RenderCommandContext::CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept
{
	m_commandList->CopyResource(destinationResource, sourceResource);
}

void RenderCommandContext::AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept
{
	m_commandList->AliasResource(beforeResource, afterResource);
}

void RenderCommandContext::TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	m_commandList->TransitionResource(resource, before, after);
}

void RenderCommandContext::UnorderedAccessBarrier(NativeResourceHandle resource) noexcept
{
	m_commandList->UnorderedAccessBarrier(resource);
}