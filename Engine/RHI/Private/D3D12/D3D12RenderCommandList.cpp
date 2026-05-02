#include "PCH.h"

#include "D3D12/D3D12RenderCommandList.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Diagnostics/D3D12PixEvents.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"

#include <array>
#include <string>
#include <vector>

D3D12RenderCommandList::D3D12RenderCommandList(D3D12RenderHardwareInterface& owner, ID3D12GraphicsCommandList* commandList) noexcept :
    m_owner(&owner), m_commandList(commandList)
{
}

ERhiBackendApi D3D12RenderCommandList::GetBackendApi() const noexcept
{
	return ERhiBackendApi::D3D12;
}

NativeGraphicsCommandListHandle D3D12RenderCommandList::GetNativeHandle() const noexcept
{
	return NativeGraphicsCommandListHandle{m_commandList};
}

bool D3D12RenderCommandList::SupportsDiagnosticScopes() const noexcept
{
	return m_commandList != nullptr && D3D12PixEvents::IsAvailable();
}

void D3D12RenderCommandList::BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	if (!SupportsDiagnosticScopes() || label.empty())
	{
		return;
	}

	const std::string ownedLabel(label);
	D3D12PixEvents::BeginEvent(m_commandList, D3D12PixEvents::ToColor(color), ownedLabel.c_str());
}

void D3D12RenderCommandList::EndDiagnosticScope() noexcept
{
	if (SupportsDiagnosticScopes())
	{
		D3D12PixEvents::EndEvent(m_commandList);
	}
}

void D3D12RenderCommandList::InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	if (!SupportsDiagnosticScopes() || label.empty())
	{
		return;
	}

	const std::string ownedLabel(label);
	D3D12PixEvents::SetMarker(m_commandList, D3D12PixEvents::ToColor(color), ownedLabel.c_str());
}

void D3D12RenderCommandList::SetDescriptorHeaps(std::uint32_t heapCount, const NativeDescriptorHeapHandle* heaps) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	std::array<ID3D12DescriptorHeap*, 2> nativeHeaps{};
	for (std::uint32_t index = 0; index < heapCount && index < nativeHeaps.size(); ++index)
	{
		nativeHeaps[index] = static_cast<ID3D12DescriptorHeap*>(heaps[index].Value);
	}

	m_commandList->SetDescriptorHeaps(heapCount, nativeHeaps.data());
}

void D3D12RenderCommandList::SetPipelineState(const RenderPipelineState& pipelineState) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const auto& nativePipelineState = static_cast<const D3D12PipelineState&>(pipelineState);
	m_commandList->SetPipelineState(nativePipelineState.Get().Get());
}

void D3D12RenderCommandList::SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
	m_commandList->SetGraphicsRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
}

void D3D12RenderCommandList::SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
	m_commandList->SetComputeRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
}

void D3D12RenderCommandList::BindGraphicsConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsShaderResource(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsUnorderedAccess(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsDescriptorTable(
    std::uint32_t rootParameterIndex,
    RhiDescriptorTableBinding tableBinding) noexcept
{
	if (m_commandList == nullptr || m_owner == nullptr || !tableBinding)
	{
		return;
	}

	m_commandList->SetGraphicsRootDescriptorTable(
	    rootParameterIndex,
	    m_owner->ResolveDescriptorTableGpuHandle(tableBinding.Table, tableBinding.DescriptorIndex));
}

void D3D12RenderCommandList::BindGraphicsDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12TypeConversions::ToGpuDescriptor(baseDescriptor));
	}
}

void D3D12RenderCommandList::SetGraphicsRootConstants(
    std::uint32_t rootParameterIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
	}
}

void D3D12RenderCommandList::BindComputeConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootConstantBufferView(rootParameterIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeShaderResource(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootShaderResourceView(rootParameterIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeUnorderedAccess(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootUnorderedAccessView(rootParameterIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeDescriptorTable(
    std::uint32_t rootParameterIndex,
    RhiDescriptorTableBinding tableBinding) noexcept
{
	if (m_commandList == nullptr || m_owner == nullptr || !tableBinding)
	{
		return;
	}

	m_commandList->SetComputeRootDescriptorTable(
	    rootParameterIndex,
	    m_owner->ResolveDescriptorTableGpuHandle(tableBinding.Table, tableBinding.DescriptorIndex));
}

void D3D12RenderCommandList::BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12TypeConversions::ToGpuDescriptor(baseDescriptor));
	}
}

void D3D12RenderCommandList::SetComputeRootConstants(
    std::uint32_t rootParameterIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
	}
}

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

void D3D12RenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeRtv = D3D12TypeConversions::ToCpuDescriptor(rtv);
	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = dsv != nullptr ? D3D12TypeConversions::ToCpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandList->OMSetRenderTargets(1, &nativeRtv, FALSE, dsv != nullptr ? &nativeDsv : nullptr);
}

void D3D12RenderCommandList::SetRenderTargets(
    std::uint32_t numRTVs,
    const RhiCpuDescriptorHandle* rtvs,
    const RhiCpuDescriptorHandle* dsv) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> nativeRtvs(numRTVs);
	for (std::uint32_t index = 0; index < numRTVs; ++index)
	{
		nativeRtvs[index] = D3D12TypeConversions::ToCpuDescriptor(rtvs[index]);
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = dsv != nullptr ? D3D12TypeConversions::ToCpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandList->OMSetRenderTargets(numRTVs, nativeRtvs.data(), FALSE, dsv != nullptr ? &nativeDsv : nullptr);
}

void D3D12RenderCommandList::ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->ClearRenderTargetView(D3D12TypeConversions::ToCpuDescriptor(rtv), color, 0, nullptr);
	}
}

void D3D12RenderCommandList::ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->ClearDepthStencilView(
		    D3D12TypeConversions::ToCpuDescriptor(dsv),
		    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		    depth,
		    stencil,
		    0,
		    nullptr);
	}
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
		m_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
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

void D3D12RenderCommandList::CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->CopyResource(D3D12TypeConversions::ToResource(destinationResource), D3D12TypeConversions::ToResource(sourceResource));
	}
}

void D3D12RenderCommandList::AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Aliasing.pResourceBefore = D3D12TypeConversions::ToResource(beforeResource);
	barrier.Aliasing.pResourceAfter = D3D12TypeConversions::ToResource(afterResource);
	m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderCommandList::TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = D3D12TypeConversions::ToResource(resource);
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12TypeConversions::ToResourceStates(before);
	barrier.Transition.StateAfter = D3D12TypeConversions::ToResourceStates(after);
	m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderCommandList::UnorderedAccessBarrier(NativeResourceHandle resource) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = D3D12TypeConversions::ToResource(resource);
	m_commandList->ResourceBarrier(1, &barrier);
}