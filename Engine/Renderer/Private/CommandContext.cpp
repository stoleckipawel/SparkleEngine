#include "PCH.h"
#include "Renderer/Public/CommandContext.h"

CommandContext::CommandContext(ID3D12GraphicsCommandList* cmdList) noexcept : m_cmdList(cmdList) {}

void CommandContext::SetPipelineState(ID3D12PipelineState* pso) noexcept
{
	m_cmdList->SetPipelineState(pso);
}

void CommandContext::SetRootSignature(ID3D12RootSignature* rootSig) noexcept
{
	m_cmdList->SetGraphicsRootSignature(rootSig);
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

void CommandContext::BindDescriptorTable(std::uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor) noexcept
{
	m_cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, baseDescriptor);
}

void CommandContext::SetDescriptorHeaps(std::uint32_t heapCount, ID3D12DescriptorHeap* const* heaps) noexcept
{
	m_cmdList->SetDescriptorHeaps(heapCount, heaps);
}

void CommandContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const D3D12_CPU_DESCRIPTOR_HANDLE* dsv) noexcept
{
	m_cmdList->OMSetRenderTargets(1, &rtv, FALSE, dsv);
}

void CommandContext::SetRenderTargets(
    std::uint32_t numRTVs,
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs,
    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv) noexcept
{
	m_cmdList->OMSetRenderTargets(numRTVs, rtvs, FALSE, dsv);
}

void CommandContext::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4]) noexcept
{
	m_cmdList->ClearRenderTargetView(rtv, color, 0, nullptr);
}

void CommandContext::ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth, std::uint8_t stencil) noexcept
{
	m_cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
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

void CommandContext::CopyResource(ID3D12Resource* destinationResource, ID3D12Resource* sourceResource) noexcept
{
	m_cmdList->CopyResource(destinationResource, sourceResource);
}

void CommandContext::AliasResource(ID3D12Resource* beforeResource, ID3D12Resource* afterResource) noexcept
{
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Aliasing.pResourceBefore = beforeResource;
	barrier.Aliasing.pResourceAfter = afterResource;

	m_cmdList->ResourceBarrier(1, &barrier);
}

void CommandContext::TransitionResource(ID3D12Resource* resource, ResourceState before, ResourceState after) noexcept
{
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = MapToD3D12State(before);
	barrier.Transition.StateAfter = MapToD3D12State(after);

	m_cmdList->ResourceBarrier(1, &barrier);
}

D3D12_RESOURCE_STATES CommandContext::MapToD3D12State(ResourceState state) noexcept
{
	return MapToD3D12ResourceState(state);
}