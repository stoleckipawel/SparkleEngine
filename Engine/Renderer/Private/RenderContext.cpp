#include "PCH.h"
#include "Renderer/Public/RenderContext.h"

RenderContext::RenderContext(ID3D12GraphicsCommandList* cmdList) noexcept : m_cmdList(cmdList) {}

void RenderContext::SetPipelineState(ID3D12PipelineState* pso) noexcept
{
	m_cmdList->SetPipelineState(pso);
}

void RenderContext::SetRootSignature(ID3D12RootSignature* rootSig) noexcept
{
	m_cmdList->SetGraphicsRootSignature(rootSig);
}

void RenderContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) noexcept
{
	m_cmdList->IASetPrimitiveTopology(topology);
}

void RenderContext::BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view) noexcept
{
	m_cmdList->IASetVertexBuffers(0, 1, &view);
}

void RenderContext::BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& view) noexcept
{
	m_cmdList->IASetIndexBuffer(&view);
}

void RenderContext::BindConstantBuffer(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept
{
	m_cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
}

void RenderContext::BindDescriptorTable(std::uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor) noexcept
{
	m_cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, baseDescriptor);
}

void RenderContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const D3D12_CPU_DESCRIPTOR_HANDLE* dsv) noexcept
{
	m_cmdList->OMSetRenderTargets(1, &rtv, FALSE, dsv);
}

void RenderContext::SetRenderTargets(
    std::uint32_t numRTVs,
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs,
    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv) noexcept
{
	m_cmdList->OMSetRenderTargets(numRTVs, rtvs, FALSE, dsv);
}

void RenderContext::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4]) noexcept
{
	m_cmdList->ClearRenderTargetView(rtv, color, 0, nullptr);
}

void RenderContext::ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth, std::uint8_t stencil) noexcept
{
	m_cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}

void RenderContext::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) noexcept
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

void RenderContext::SetScissorRect(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom) noexcept
{
	D3D12_RECT scissor{};
	scissor.left = static_cast<LONG>(left);
	scissor.top = static_cast<LONG>(top);
	scissor.right = static_cast<LONG>(right);
	scissor.bottom = static_cast<LONG>(bottom);

	m_cmdList->RSSetScissorRects(1, &scissor);
}

void RenderContext::DrawIndexedInstanced(
    std::uint32_t indexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	m_cmdList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void RenderContext::DrawInstanced(
    std::uint32_t vertexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	m_cmdList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void RenderContext::TransitionResource(ID3D12Resource* resource, ResourceState before, ResourceState after) noexcept
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

D3D12_RESOURCE_STATES RenderContext::MapToD3D12State(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Common:
			return D3D12_RESOURCE_STATE_COMMON;
		case ResourceState::RenderTarget:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case ResourceState::DepthWrite:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case ResourceState::DepthRead:
			return D3D12_RESOURCE_STATE_DEPTH_READ;
		case ResourceState::ShaderResource:
			return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case ResourceState::UnorderedAccess:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case ResourceState::CopySource:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case ResourceState::CopyDest:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case ResourceState::Present:
			return D3D12_RESOURCE_STATE_PRESENT;
		default:
			return D3D12_RESOURCE_STATE_COMMON;
	}
}
