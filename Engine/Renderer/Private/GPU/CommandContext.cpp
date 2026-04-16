#include "PCH.h"
#include "GPU/CommandContext.h"

CommandContext::CommandContext(RenderCommandList& commandList) noexcept : m_commandList(&commandList) {}

void CommandContext::SetPipelineState(const RenderPipelineState& pipelineState) noexcept
{
	m_commandList->SetPipelineState(pipelineState);
}

void CommandContext::SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_commandList->SetGraphicsBindingLayout(bindingLayout);
}

void CommandContext::SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_commandList->SetComputeBindingLayout(bindingLayout);
}

void CommandContext::SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept
{
	m_commandList->SetPrimitiveTopology(topology);
}

void CommandContext::BindVertexBuffer(const RhiVertexBufferView& view) noexcept
{
	m_commandList->BindVertexBuffer(view);
}

void CommandContext::BindIndexBuffer(const RhiIndexBufferView& view) noexcept
{
	m_commandList->BindIndexBuffer(view);
}

void CommandContext::BindConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsConstantBuffer(rootParameterIndex, gpuAddress);
}

void CommandContext::SetRoot32BitConstants(
    std::uint32_t rootParameterIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_commandList->SetGraphicsRootConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void CommandContext::BindRootShaderResourceView(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsShaderResource(rootParameterIndex, gpuAddress);
}

void CommandContext::BindRootUnorderedAccessView(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindGraphicsUnorderedAccess(rootParameterIndex, gpuAddress);
}

void CommandContext::BindDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_commandList->BindGraphicsDescriptorTable(rootParameterIndex, baseDescriptor);
}

void CommandContext::BindDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableHandle tableHandle) noexcept
{
	m_commandList->BindGraphicsDescriptorTable(rootParameterIndex, tableHandle);
}

void CommandContext::BindComputeRootConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeConstantBuffer(rootParameterIndex, gpuAddress);
}

void CommandContext::BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_commandList->BindComputeDescriptorTable(rootParameterIndex, baseDescriptor);
}

void CommandContext::BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableHandle tableHandle) noexcept
{
	m_commandList->BindComputeDescriptorTable(rootParameterIndex, tableHandle);
}

void CommandContext::SetComputeRoot32BitConstants(
    std::uint32_t rootParameterIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_commandList->SetComputeRootConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void CommandContext::BindComputeRootShaderResourceView(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeShaderResource(rootParameterIndex, gpuAddress);
}

void CommandContext::BindComputeRootUnorderedAccessView(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	m_commandList->BindComputeUnorderedAccess(rootParameterIndex, gpuAddress);
}

void CommandContext::SetDescriptorHeaps(std::uint32_t heapCount, const NativeDescriptorHeapHandle* heaps) noexcept
{
	m_commandList->SetDescriptorHeaps(heapCount, heaps);
}

void CommandContext::SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept
{
	m_commandList->SetRenderTarget(rtv, dsv);
}

void CommandContext::SetRenderTargets(
    std::uint32_t numRTVs,
    const RhiCpuDescriptorHandle* rtvs,
    const RhiCpuDescriptorHandle* dsv) noexcept
{
	m_commandList->SetRenderTargets(numRTVs, rtvs, dsv);
}

void CommandContext::ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept
{
	m_commandList->ClearRenderTarget(rtv, color);
}

void CommandContext::ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil) noexcept
{
	m_commandList->ClearDepthStencil(dsv, depth, stencil);
}

void CommandContext::SetViewport(const RhiViewport& viewport) noexcept
{
	m_commandList->SetViewport(viewport);
}

void CommandContext::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) noexcept
{
	SetViewport(RhiViewport{.X = x, .Y = y, .Width = width, .Height = height, .MinDepth = minDepth, .MaxDepth = maxDepth});
}

void CommandContext::SetScissorRect(const RhiRect& scissorRect) noexcept
{
	m_commandList->SetScissorRect(scissorRect);
}

void CommandContext::SetScissorRect(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom) noexcept
{
	SetScissorRect(RhiRect{.Left = left, .Top = top, .Right = right, .Bottom = bottom});
}

void CommandContext::DrawIndexedInstanced(
    std::uint32_t indexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	m_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void CommandContext::DrawInstanced(
    std::uint32_t vertexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void CommandContext::Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void CommandContext::CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept
{
	m_commandList->CopyResource(destinationResource, sourceResource);
}

void CommandContext::AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept
{
	m_commandList->AliasResource(beforeResource, afterResource);
}

void CommandContext::TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	m_commandList->TransitionResource(resource, before, after);
}

void CommandContext::UnorderedAccessBarrier(NativeResourceHandle resource) noexcept
{
	m_commandList->UnorderedAccessBarrier(resource);
}