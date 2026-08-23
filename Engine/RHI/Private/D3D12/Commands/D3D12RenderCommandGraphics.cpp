#include "PCH.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"

#include "D3D12/D3D12TypeConversions.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <vector>

static const auto g_d3d12RenderCommandListLogger = Logging::GetOrCreateLogger("RHI.D3D12.CommandList");

void D3D12RenderCommandList::SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->IASetPrimitiveTopology(D3D12TypeConversions::ToPrimitiveTopology(topology));
	}
}

void D3D12RenderCommandList::BindVertexBuffer(const RhiVertexBufferView& view) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_VERTEX_BUFFER_VIEW nativeView{
	    .BufferLocation = view.BufferLocation,
	    .SizeInBytes = view.SizeInBytes,
	    .StrideInBytes = view.StrideInBytes};
	m_commandList->IASetVertexBuffers(0, 1, &nativeView);
}

void D3D12RenderCommandList::BindIndexBuffer(const RhiIndexBufferView& view) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_INDEX_BUFFER_VIEW nativeView{
	    .BufferLocation = view.BufferLocation,
	    .SizeInBytes = view.SizeInBytes,
	    .Format = D3D12TypeConversions::ToIndexFormat(view.Format)};
	m_commandList->IASetIndexBuffer(&nativeView);
}

void D3D12RenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle renderTarget, const RhiCpuDescriptorHandle* depthStencil) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeRtv = D3D12TypeConversions::ToCpuDescriptor(renderTarget);
	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv =
	    depthStencil != nullptr ? D3D12TypeConversions::ToCpuDescriptor(*depthStencil) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandList->OMSetRenderTargets(1, &nativeRtv, FALSE, depthStencil != nullptr ? &nativeDsv : nullptr);
}

void D3D12RenderCommandList::SetRenderTargets(
    std::uint32_t renderTargetCount,
    const RhiCpuDescriptorHandle* renderTargets,
    const RhiCpuDescriptorHandle* depthStencil) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}
	if (renderTargetCount > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT || (renderTargetCount != 0 && renderTargets == nullptr))
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 SetRenderTargets received an invalid render-target count or array.");
	}

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> nativeRtvs(renderTargetCount);
	for (std::uint32_t index = 0; index < renderTargetCount; ++index)
	{
		nativeRtvs[index] = D3D12TypeConversions::ToCpuDescriptor(renderTargets[index]);
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv =
	    depthStencil != nullptr ? D3D12TypeConversions::ToCpuDescriptor(*depthStencil) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandList->OMSetRenderTargets(renderTargetCount, nativeRtvs.data(), FALSE, depthStencil != nullptr ? &nativeDsv : nullptr);
}

void D3D12RenderCommandList::ClearRenderTarget(RhiCpuDescriptorHandle renderTarget, const float color[4]) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->ClearRenderTargetView(D3D12TypeConversions::ToCpuDescriptor(renderTarget), color, 0, nullptr);
	}
}

void D3D12RenderCommandList::ClearDepthStencil(RhiCpuDescriptorHandle depthStencil, float depth, std::uint8_t stencil) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->ClearDepthStencilView(
		    D3D12TypeConversions::ToCpuDescriptor(depthStencil),
		    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		    depth,
		    stencil,
		    0,
		    nullptr);
	}
}

void D3D12RenderCommandList::EndRasterPass() noexcept
{
}

void D3D12RenderCommandList::SetViewport(const RhiViewport& viewport) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_VIEWPORT nativeViewport{
	    .TopLeftX = viewport.X,
	    .TopLeftY = viewport.Y,
	    .Width = viewport.Width,
	    .Height = viewport.Height,
	    .MinDepth = viewport.MinDepth,
	    .MaxDepth = viewport.MaxDepth};
	m_commandList->RSSetViewports(1, &nativeViewport);
}

void D3D12RenderCommandList::SetScissorRect(const RhiRect& rect) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_RECT nativeRect{.left = rect.Left, .top = rect.Top, .right = rect.Right, .bottom = rect.Bottom};
	m_commandList->RSSetScissorRects(1, &nativeRect);
}

void D3D12RenderCommandList::DrawIndexedInstanced(
    std::uint32_t indexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList
		    ->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
}

void D3D12RenderCommandList::DrawInstanced(
    std::uint32_t vertexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}
}

void D3D12RenderCommandList::Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}
}
