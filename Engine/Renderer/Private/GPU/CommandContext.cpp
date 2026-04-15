#include "PCH.h"
#include "GPU/CommandContext.h"

#include "GPU/ResourceStateD3D12.h"

#include <vector>

namespace
{
	D3D12_GPU_DESCRIPTOR_HANDLE ToD3D12GpuDescriptor(RhiGpuDescriptorHandle handle) noexcept
	{
		D3D12_GPU_DESCRIPTOR_HANDLE nativeHandle{};
		nativeHandle.ptr = handle.Value;
		return nativeHandle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ToD3D12CpuDescriptor(RhiCpuDescriptorHandle handle) noexcept
	{
		D3D12_CPU_DESCRIPTOR_HANDLE nativeHandle{};
		nativeHandle.ptr = handle.Value;
		return nativeHandle;
	}

	ID3D12Resource* ToD3D12Resource(NativeResourceHandle handle) noexcept
	{
		return static_cast<ID3D12Resource*>(handle.Value);
	}
}  // namespace

CommandContext::CommandContext(ID3D12GraphicsCommandList* cmdList) noexcept : m_cmdList(cmdList) {}

void CommandContext::SetPipelineState(ID3D12PipelineState* pso) noexcept
{
	m_cmdList->SetPipelineState(pso);
}

void CommandContext::SetRootSignature(ID3D12RootSignature* rootSig) noexcept
{
	m_cmdList->SetGraphicsRootSignature(rootSig);
}

void CommandContext::SetComputeRootSignature(ID3D12RootSignature* rootSig) noexcept
{
	m_cmdList->SetComputeRootSignature(rootSig);
}

void CommandContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) noexcept
{
	m_cmdList->IASetPrimitiveTopology(topology);
}

void CommandContext::BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view) noexcept
{
	m_cmdList->IASetVertexBuffers(0, 1, &view);
}

void CommandContext::BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& view) noexcept
{
	m_cmdList->IASetIndexBuffer(&view);
}

void CommandContext::BindConstantBuffer(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept
{
	m_cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
}

void CommandContext::SetRoot32BitConstants(
    std::uint32_t rootParameterIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_cmdList->SetGraphicsRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void CommandContext::BindRootShaderResourceView(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept
{
	m_cmdList->SetGraphicsRootShaderResourceView(rootParameterIndex, gpuAddress);
}

void CommandContext::BindRootUnorderedAccessView(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept
{
	m_cmdList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, gpuAddress);
}

void CommandContext::BindDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, ToD3D12GpuDescriptor(baseDescriptor));
}

void CommandContext::BindComputeRootConstantBuffer(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept
{
	m_cmdList->SetComputeRootConstantBufferView(rootParameterIndex, gpuAddress);
}

void CommandContext::BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	m_cmdList->SetComputeRootDescriptorTable(rootParameterIndex, ToD3D12GpuDescriptor(baseDescriptor));
}

void CommandContext::SetComputeRoot32BitConstants(
    std::uint32_t rootParameterIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	m_cmdList->SetComputeRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void CommandContext::BindComputeRootShaderResourceView(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept
{
	m_cmdList->SetComputeRootShaderResourceView(rootParameterIndex, gpuAddress);
}

void CommandContext::BindComputeRootUnorderedAccessView(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept
{
	m_cmdList->SetComputeRootUnorderedAccessView(rootParameterIndex, gpuAddress);
}

void CommandContext::SetDescriptorHeaps(std::uint32_t heapCount, ID3D12DescriptorHeap* const* heaps) noexcept
{
	m_cmdList->SetDescriptorHeaps(heapCount, heaps);
}

void CommandContext::SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept
{
	const D3D12_CPU_DESCRIPTOR_HANDLE nativeRtv = ToD3D12CpuDescriptor(rtv);
	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = dsv != nullptr ? ToD3D12CpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_cmdList->OMSetRenderTargets(1, &nativeRtv, FALSE, dsv != nullptr ? &nativeDsv : nullptr);
}

void CommandContext::SetRenderTargets(
    std::uint32_t numRTVs,
    const RhiCpuDescriptorHandle* rtvs,
    const RhiCpuDescriptorHandle* dsv) noexcept
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> nativeRtvs(numRTVs);
	for (std::uint32_t index = 0; index < numRTVs; ++index)
	{
		nativeRtvs[index] = ToD3D12CpuDescriptor(rtvs[index]);
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = dsv != nullptr ? ToD3D12CpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_cmdList->OMSetRenderTargets(numRTVs, nativeRtvs.data(), FALSE, dsv != nullptr ? &nativeDsv : nullptr);
}

void CommandContext::ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept
{
	m_cmdList->ClearRenderTargetView(ToD3D12CpuDescriptor(rtv), color, 0, nullptr);
}

void CommandContext::ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil) noexcept
{
	m_cmdList->ClearDepthStencilView(
	    ToD3D12CpuDescriptor(dsv),
	    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
	    depth,
	    stencil,
	    0,
	    nullptr);
}

void CommandContext::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) noexcept
{
	D3D12_VIEWPORT viewport{};
	viewport.TopLeftX = x;
	viewport.TopLeftY = y;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = minDepth;
	viewport.MaxDepth = maxDepth;

	m_cmdList->RSSetViewports(1, &viewport);
}

void CommandContext::SetScissorRect(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom) noexcept
{
	D3D12_RECT scissor{};
	scissor.left = static_cast<LONG>(left);
	scissor.top = static_cast<LONG>(top);
	scissor.right = static_cast<LONG>(right);
	scissor.bottom = static_cast<LONG>(bottom);

	m_cmdList->RSSetScissorRects(1, &scissor);
}

void CommandContext::DrawIndexedInstanced(
    std::uint32_t indexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	m_cmdList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void CommandContext::DrawInstanced(
    std::uint32_t vertexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	m_cmdList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void CommandContext::Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	m_cmdList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void CommandContext::CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept
{
	m_cmdList->CopyResource(ToD3D12Resource(destinationResource), ToD3D12Resource(sourceResource));
}

void CommandContext::AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept
{
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Aliasing.pResourceBefore = ToD3D12Resource(beforeResource);
	barrier.Aliasing.pResourceAfter = ToD3D12Resource(afterResource);

	m_cmdList->ResourceBarrier(1, &barrier);
}

void CommandContext::TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = ToD3D12Resource(resource);
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = MapToD3D12State(before);
	barrier.Transition.StateAfter = MapToD3D12State(after);

	m_cmdList->ResourceBarrier(1, &barrier);
}

void CommandContext::UnorderedAccessBarrier(NativeResourceHandle resource) noexcept
{
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = ToD3D12Resource(resource);

	m_cmdList->ResourceBarrier(1, &barrier);
}

D3D12_RESOURCE_STATES CommandContext::MapToD3D12State(ResourceState state) noexcept
{
	return MapToD3D12ResourceState(state);
}