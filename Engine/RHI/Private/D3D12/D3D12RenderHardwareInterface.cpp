#include "PCH.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

#include "D3D12/D3D12Rhi.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "Resources/Texture.h"
#include "D3D12/Textures/TextureFactory.h"
#include "D3D12/Textures/TextureLoader.h"

#include <d3d12.h>
#include <cstring>
#include <string>
#include <vector>

namespace
{
	struct OwnedHeapState
	{
		Microsoft::WRL::ComPtr<ID3D12Heap> Heap;
	};

	struct OwnedResourceState
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	};

	ID3D12GraphicsCommandList* ToD3D12GraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept
	{
		return static_cast<ID3D12GraphicsCommandList*>(handle.Value);
	}

	ID3D12Resource* ToD3D12Resource(NativeResourceHandle handle) noexcept
	{
		return static_cast<ID3D12Resource*>(handle.Value);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ToD3D12CpuDescriptor(RhiCpuDescriptorHandle handle) noexcept
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE{handle.Value};
	}

	D3D12_GPU_DESCRIPTOR_HANDLE ToD3D12GpuDescriptor(RhiGpuDescriptorHandle handle) noexcept
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE{handle.Value};
	}

	D3D12_DESCRIPTOR_HEAP_TYPE ToD3D12DescriptorHeapType(RhiDescriptorHeapType heapType) noexcept
	{
		switch (heapType)
		{
			case RhiDescriptorHeapType::RenderTarget:
				return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			case RhiDescriptorHeapType::DepthStencil:
				return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			case RhiDescriptorHeapType::Sampler:
				return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			case RhiDescriptorHeapType::ShaderResource:
			default:
				return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		}
	}

	D3D12_RESOURCE_STATES ToD3D12ResourceState(ResourceState state) noexcept
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
				return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
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

	D3D12_PRIMITIVE_TOPOLOGY ToD3D12PrimitiveTopology(RhiPrimitiveTopology topology) noexcept
	{
		switch (topology)
		{
			case RhiPrimitiveTopology::TriangleList:
			default:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
	}

	DXGI_FORMAT ToD3D12IndexFormat(RhiIndexFormat format) noexcept
	{
		switch (format)
		{
			case RhiIndexFormat::UInt16:
				return DXGI_FORMAT_R16_UINT;
			case RhiIndexFormat::UInt32:
			default:
				return DXGI_FORMAT_R32_UINT;
		}
	}

	D3D12_RESOURCE_DESC BuildTextureResourceDesc(const RhiTextureResourceDesc& desc) noexcept
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = static_cast<UINT64>(desc.Width);
		resourceDesc.Height = desc.Height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = desc.MipLevels;
		resourceDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (desc.AllowRenderTarget)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (desc.AllowDepthStencil)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (desc.AllowUnorderedAccess)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		return resourceDesc;
	}

	D3D12_RESOURCE_DESC BuildBufferResourceDesc(const RhiBufferResourceDesc& desc) noexcept
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = desc.SizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = desc.AllowUnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		return resourceDesc;
	}

	D3D12_HEAP_FLAGS ToHeapFlags(RhiTransientAllocationPool pool) noexcept
	{
		switch (pool)
		{
			case RhiTransientAllocationPool::Buffer:
				return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			case RhiTransientAllocationPool::Color:
			case RhiTransientAllocationPool::Depth:
			default:
				return D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
		}
	}

	D3D12_CLEAR_VALUE BuildClearValue(const RhiOptimizedClearValue& clearValue) noexcept
	{
		D3D12_CLEAR_VALUE nativeClearValue{};
		nativeClearValue.Format = D3D12TypeConversions::ToDxgiFormat(clearValue.Format);
		if (clearValue.ValueType == RhiOptimizedClearValue::Type::DepthStencil)
		{
			nativeClearValue.DepthStencil.Depth = clearValue.Depth;
			nativeClearValue.DepthStencil.Stencil = clearValue.Stencil;
		}
		else
		{
			for (std::size_t index = 0; index < clearValue.Color.size(); ++index)
			{
				nativeClearValue.Color[index] = clearValue.Color[index];
			}
		}
		return nativeClearValue;
	}

	D3D12_HEAP_PROPERTIES BuildUploadHeapProperties() noexcept
	{
		D3D12_HEAP_PROPERTIES properties{};
		properties.Type = D3D12_HEAP_TYPE_UPLOAD;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		properties.CreationNodeMask = 1;
		properties.VisibleNodeMask = 1;
		return properties;
	}

	std::wstring CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName) noexcept
	{
		return debugName.empty() ? std::wstring(fallbackName) : std::wstring(debugName);
	}

	OwnedHeapState* ToOwnedHeapState(RhiOwnedHeapHandle handle) noexcept
	{
		return static_cast<OwnedHeapState*>(handle.Value);
	}

	const OwnedResourceState* ToConstOwnedResourceState(RhiOwnedResourceHandle handle) noexcept
	{
		return static_cast<const OwnedResourceState*>(handle.Value);
	}

	OwnedResourceState* ToOwnedResourceState(RhiOwnedResourceHandle handle) noexcept
	{
		return static_cast<OwnedResourceState*>(handle.Value);
	}

	bool ResourceSupportsUnorderedAccess(ID3D12Resource* resource) noexcept
	{
		return resource != nullptr && (resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;
	}
}

class D3D12RenderHardwareInterface::D3D12RenderCommandList final : public RenderCommandList
{
  public:
	D3D12RenderCommandList(D3D12RenderHardwareInterface& owner, ID3D12GraphicsCommandList* commandList) noexcept :
	    m_owner(&owner), m_commandList(commandList)
	{
	}

	RhiBackendApi GetBackendApi() const noexcept override { return RhiBackendApi::D3D12; }
	NativeGraphicsCommandListHandle GetNativeHandle() const noexcept override { return NativeGraphicsCommandListHandle{m_commandList}; }

	void SetDescriptorHeaps(std::uint32_t heapCount, const NativeDescriptorHeapHandle* heaps) noexcept override
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

	void SetPipelineState(const RenderPipelineState& pipelineState) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const auto& nativePipelineState = static_cast<const D3D12PipelineState&>(pipelineState);
		m_commandList->SetPipelineState(nativePipelineState.Get().Get());
	}

	void SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
		m_commandList->SetGraphicsRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
	}

	void SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
		m_commandList->SetComputeRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
	}

	void BindGraphicsConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
		}
	}

	void BindGraphicsShaderResource(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, gpuAddress);
		}
	}

	void BindGraphicsUnorderedAccess(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, gpuAddress);
		}
	}

	void BindGraphicsDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableHandle tableHandle) noexcept override
	{
		if (m_commandList == nullptr || m_owner == nullptr || !tableHandle)
		{
			return;
		}

		m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, m_owner->ResolveDescriptorTableGpuHandle(tableHandle));
	}

	void BindGraphicsDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, ToD3D12GpuDescriptor(baseDescriptor));
		}
	}

	void SetGraphicsRootConstants(
	    std::uint32_t rootParameterIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
		}
	}

	void BindComputeConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRootConstantBufferView(rootParameterIndex, gpuAddress);
		}
	}

	void BindComputeShaderResource(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRootShaderResourceView(rootParameterIndex, gpuAddress);
		}
	}

	void BindComputeUnorderedAccess(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRootUnorderedAccessView(rootParameterIndex, gpuAddress);
		}
	}

	void BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableHandle tableHandle) noexcept override
	{
		if (m_commandList == nullptr || m_owner == nullptr || !tableHandle)
		{
			return;
		}

		m_commandList->SetComputeRootDescriptorTable(rootParameterIndex, m_owner->ResolveDescriptorTableGpuHandle(tableHandle));
	}

	void BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRootDescriptorTable(rootParameterIndex, ToD3D12GpuDescriptor(baseDescriptor));
		}
	}

	void SetComputeRootConstants(
	    std::uint32_t rootParameterIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
		}
	}

	void SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->IASetPrimitiveTopology(ToD3D12PrimitiveTopology(topology));
		}
	}

	void BindVertexBuffer(const RhiVertexBufferView& view) noexcept override
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

	void BindIndexBuffer(const RhiIndexBufferView& view) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const D3D12_INDEX_BUFFER_VIEW nativeView{
		    .BufferLocation = view.BufferLocation,
		    .SizeInBytes = view.SizeInBytes,
		    .Format = ToD3D12IndexFormat(view.Format)};
		m_commandList->IASetIndexBuffer(&nativeView);
	}

	void SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE nativeRtv = ToD3D12CpuDescriptor(rtv);
		const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = dsv != nullptr ? ToD3D12CpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
		m_commandList->OMSetRenderTargets(1, &nativeRtv, FALSE, dsv != nullptr ? &nativeDsv : nullptr);
	}

	void SetRenderTargets(
	    std::uint32_t numRTVs,
	    const RhiCpuDescriptorHandle* rtvs,
	    const RhiCpuDescriptorHandle* dsv) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> nativeRtvs(numRTVs);
		for (std::uint32_t index = 0; index < numRTVs; ++index)
		{
			nativeRtvs[index] = ToD3D12CpuDescriptor(rtvs[index]);
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = dsv != nullptr ? ToD3D12CpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
		m_commandList->OMSetRenderTargets(numRTVs, nativeRtvs.data(), FALSE, dsv != nullptr ? &nativeDsv : nullptr);
	}

	void ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->ClearRenderTargetView(ToD3D12CpuDescriptor(rtv), color, 0, nullptr);
		}
	}

	void ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->ClearDepthStencilView(
			    ToD3D12CpuDescriptor(dsv),
			    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			    depth,
			    stencil,
			    0,
			    nullptr);
		}
	}

	void SetViewport(const RhiViewport& viewport) noexcept override
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

	void SetScissorRect(const RhiRect& rect) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const D3D12_RECT nativeRect{
		    .left = rect.Left,
		    .top = rect.Top,
		    .right = rect.Right,
		    .bottom = rect.Bottom};
		m_commandList->RSSetScissorRects(1, &nativeRect);
	}

	void DrawIndexedInstanced(
	    std::uint32_t indexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startIndexLocation,
	    std::int32_t baseVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->DrawIndexedInstanced(
			    indexCountPerInstance,
			    instanceCount,
			    startIndexLocation,
			    baseVertexLocation,
			    startInstanceLocation);
		}
	}

	void DrawInstanced(
	    std::uint32_t vertexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
		}
	}

	void Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
		}
	}

	void CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->CopyResource(ToD3D12Resource(destinationResource), ToD3D12Resource(sourceResource));
		}
	}

	void AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Aliasing.pResourceBefore = ToD3D12Resource(beforeResource);
		barrier.Aliasing.pResourceAfter = ToD3D12Resource(afterResource);
		m_commandList->ResourceBarrier(1, &barrier);
	}

	void TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = ToD3D12Resource(resource);
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = ToD3D12ResourceState(before);
		barrier.Transition.StateAfter = ToD3D12ResourceState(after);
		m_commandList->ResourceBarrier(1, &barrier);
	}

	void UnorderedAccessBarrier(NativeResourceHandle resource) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = ToD3D12Resource(resource);
		m_commandList->ResourceBarrier(1, &barrier);
	}

  private:
	D3D12RenderHardwareInterface* m_owner = nullptr;
	ID3D12GraphicsCommandList* m_commandList = nullptr;
};

D3D12RenderHardwareInterface::D3D12RenderHardwareInterface(
    D3D12Rhi& rhi,
    D3D12DescriptorHeapManager& descriptorHeapManager,
	D3D12SwapChain& swapChain,
	D3D12ConstantBufferManager& constantBufferManager) noexcept :
	m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager), m_swapChain(&swapChain), m_constantBufferManager(&constantBufferManager)
{
	for (std::uint32_t frameIndex = 0; frameIndex < RenderConfig::FramesInFlight; ++frameIndex)
	{
		m_commandLists[frameIndex] = std::make_unique<D3D12RenderCommandList>(*this, rhi.GetCommandList(frameIndex).Get());
	}
}

RhiBackendApi D3D12RenderHardwareInterface::GetBackendApi() const noexcept
{
	return RhiBackendApi::D3D12;
}

std::uint32_t D3D12RenderHardwareInterface::GetCurrentFrameIndex() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetCurrentFrameIndex() : 0u;
}

NativeGraphicsDeviceHandle D3D12RenderHardwareInterface::GetDeviceHandle() const noexcept
{
	return NativeGraphicsDeviceHandle{m_rhi != nullptr ? m_rhi->GetDevice().Get() : nullptr};
}

NativeGraphicsQueueHandle D3D12RenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi != nullptr ? m_rhi->GetCommandQueue().Get() : nullptr};
}

RenderCommandList& D3D12RenderHardwareInterface::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return *m_commandLists[frameIndex];
}

NativeGraphicsCommandListHandle D3D12RenderHardwareInterface::GetGraphicsCommandListHandle(std::uint32_t frameIndex) const noexcept
{
	return NativeGraphicsCommandListHandle{m_rhi != nullptr ? m_rhi->GetCommandList(frameIndex).Get() : nullptr};
}

std::unique_ptr<RenderBindingLayout> D3D12RenderHardwareInterface::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	if (m_rhi == nullptr || desc.ParameterLayout == nullptr || desc.ShaderPackage == nullptr)
	{
		return {};
	}

	return D3D12BindingLayoutCompiler::Compile(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.VertexShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12PipelineState>(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.ComputeShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12PipelineState>(*m_rhi, desc);
}

void D3D12RenderHardwareInterface::SetShaderVisibleDescriptorHeaps(RenderCommandList& commandList) const noexcept
{
	if (m_descriptorHeapManager != nullptr)
	{
		m_descriptorHeapManager->SetShaderVisibleHeaps(commandList);
	}
}

NativeDescriptorHeapHandle D3D12RenderHardwareInterface::GetShaderResourceHeapHandle() const noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	D3D12DescriptorHeap* heap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return NativeDescriptorHeapHandle{heap != nullptr ? heap->GetRaw() : nullptr};
}

RhiDescriptorAllocation D3D12RenderHardwareInterface::AllocateDescriptor(RhiDescriptorHeapType heapType)
{
	RhiDescriptorAllocation allocation{};
	if (m_descriptorHeapManager == nullptr)
	{
		return allocation;
	}

	const D3D12_DESCRIPTOR_HEAP_TYPE nativeType = ToNativeDescriptorHeapType(heapType);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(nativeType, cpuHandle, gpuHandle);
	allocation.CpuHandle = RhiCpuDescriptorHandle{cpuHandle.ptr};
	allocation.GpuHandle = RhiGpuDescriptorHandle{gpuHandle.ptr};
	return allocation;
}

void D3D12RenderHardwareInterface::ReleaseDescriptor(RhiDescriptorHeapType heapType, const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorHeapManager == nullptr || !allocation.CpuHandle)
	{
		return;
	}

	m_descriptorHeapManager->FreeHandle(
	    ToNativeDescriptorHeapType(heapType),
	    D3D12_CPU_DESCRIPTOR_HANDLE{allocation.CpuHandle.Value},
	    D3D12_GPU_DESCRIPTOR_HANDLE{allocation.GpuHandle.Value});
}

RhiDescriptorTableHandle D3D12RenderHardwareInterface::AllocateDescriptorTable(
	RhiDescriptorHeapType heapType,
	std::uint32_t descriptorCount)
{
	if (m_descriptorHeapManager == nullptr || descriptorCount == 0)
	{
		return {};
	}

	const D3D12DescriptorHandle nativeHandle =
	    m_descriptorHeapManager->AllocateContiguous(ToD3D12DescriptorHeapType(heapType), descriptorCount);
	if (!nativeHandle.IsValid())
	{
		return {};
	}

	DescriptorTableRecord record{};
	record.heapType = heapType;
	record.descriptorCount = descriptorCount;
	record.nativeHandle = nativeHandle;

	if (!m_freeDescriptorTableIndices.empty())
	{
		const std::uint32_t recordIndex = m_freeDescriptorTableIndices.back();
		m_freeDescriptorTableIndices.pop_back();
		m_descriptorTableRecords[recordIndex] = record;
		return RhiDescriptorTableHandle{recordIndex + 1u};
	}

	m_descriptorTableRecords.push_back(record);
	return RhiDescriptorTableHandle{static_cast<std::uint32_t>(m_descriptorTableRecords.size())};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetDescriptorTableCpuHandle(
	RhiDescriptorTableHandle tableHandle,
	std::uint32_t descriptorIndex) const noexcept
{
	return RhiCpuDescriptorHandle{ResolveDescriptorTableCpuHandle(tableHandle, descriptorIndex).ptr};
}

void D3D12RenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || m_descriptorHeapManager == nullptr || !record->IsAllocated())
	{
		return;
	}

	m_descriptorHeapManager->FreeContiguous(
	    ToD3D12DescriptorHeapType(record->heapType),
	    record->nativeHandle,
	    record->descriptorCount);
	*record = DescriptorTableRecord{};
	m_freeDescriptorTableIndices.push_back(tableHandle.Value - 1u);
}

void D3D12RenderHardwareInterface::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	outCpuHandle = {};
	outGpuHandle = {};
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpuHandle, gpuHandle);
	outCpuHandle.Value = cpuHandle.ptr;
	outGpuHandle.Value = gpuHandle.ptr;
}

void D3D12RenderHardwareInterface::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	if (m_descriptorHeapManager == nullptr || !cpuHandle)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE nativeCpuHandle{};
	nativeCpuHandle.ptr = cpuHandle.Value;
	D3D12_GPU_DESCRIPTOR_HANDLE nativeGpuHandle{};
	nativeGpuHandle.ptr = gpuHandle.Value;
	m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nativeCpuHandle, nativeGpuHandle);
}

const PerFrameConstantBufferData& D3D12RenderHardwareInterface::GetPerFrameConstantData() const noexcept
{
	static const PerFrameConstantBufferData emptyData{};
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameData() : emptyData;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::GetPerFrameConstantGpuAddress() const noexcept
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameGpuAddress() : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocatePerView(data) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->UpdatePerObjectVS(data) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->UpdatePerObjectPS(data) : 0;
}

RhiDescriptorTableHandle D3D12RenderHardwareInterface::GetSamplerTableHandle() const noexcept
{
	return m_samplerTableHandle;
}

RhiViewport D3D12RenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultViewport() : RhiViewport{};
}

RhiRect D3D12RenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return m_swapChain != nullptr ? RhiCpuDescriptorHandle{m_swapChain->GetCPUHandle().ptr} : RhiCpuDescriptorHandle{};
}

NativeResourceHandle D3D12RenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return NativeResourceHandle{m_swapChain != nullptr ? m_swapChain->GetCurrentResource() : nullptr};
}

std::unique_ptr<Texture> D3D12RenderHardwareInterface::CreateTextureFromPath(const std::filesystem::path& texturePath) const
{
	if (m_rhi == nullptr || m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	TextureLoadResult loadResult =
	    TextureLoader::Load(texturePath);
	if (!loadResult.IsValid())
	{
		return {};
	}

	std::unique_ptr<TextureFactory> textureFactory = TextureFactory::Create(*m_rhi, *m_descriptorHeapManager);
	return textureFactory != nullptr ? textureFactory->CreateTexture(std::move(loadResult)) : std::unique_ptr<Texture>{};
}

bool D3D12RenderHardwareInterface::CreateVertexBuffer(
	const void* data,
	std::size_t sizeInBytes,
	std::uint32_t strideInBytes,
	std::wstring_view debugName,
	RhiOwnedResourceHandle& outResource,
	RhiVertexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	const D3D12_HEAP_PROPERTIES heapProperties = BuildUploadHeapProperties();
	auto ownedResource = std::make_unique<OwnedResourceState>();
	if (FAILED(m_rhi->GetDevice()->CreateCommittedResource(
	        &heapProperties,
	        D3D12_HEAP_FLAG_NONE,
	        &resourceDesc,
	        D3D12_RESOURCE_STATE_GENERIC_READ,
	        nullptr,
	        IID_PPV_ARGS(ownedResource->Resource.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

	ownedResource->Resource->SetName(CopyDebugName(debugName, L"VertexBuffer").c_str());
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Resource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Resource->Unmap(0, nullptr);

	outView = RhiVertexBufferView{
	    .BufferLocation = ownedResource->Resource->GetGPUVirtualAddress(),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .StrideInBytes = strideInBytes};
	outResource = RhiOwnedResourceHandle{ownedResource.release()};
	return true;
}

bool D3D12RenderHardwareInterface::CreateIndexBuffer(
	const void* data,
	std::size_t sizeInBytes,
	RhiIndexFormat format,
	std::wstring_view debugName,
	RhiOwnedResourceHandle& outResource,
	RhiIndexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || data == nullptr || sizeInBytes == 0)
	{
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	const D3D12_HEAP_PROPERTIES heapProperties = BuildUploadHeapProperties();
	auto ownedResource = std::make_unique<OwnedResourceState>();
	if (FAILED(m_rhi->GetDevice()->CreateCommittedResource(
	        &heapProperties,
	        D3D12_HEAP_FLAG_NONE,
	        &resourceDesc,
	        D3D12_RESOURCE_STATE_GENERIC_READ,
	        nullptr,
	        IID_PPV_ARGS(ownedResource->Resource.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

	ownedResource->Resource->SetName(CopyDebugName(debugName, L"IndexBuffer").c_str());
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Resource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Resource->Unmap(0, nullptr);

	outView = RhiIndexBufferView{
	    .BufferLocation = ownedResource->Resource->GetGPUVirtualAddress(),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .Format = format};
	outResource = RhiOwnedResourceHandle{ownedResource.release()};
	return true;
}

void D3D12RenderHardwareInterface::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
{
	delete ToOwnedResourceState(resource);
}

NativeResourceHandle D3D12RenderHardwareInterface::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	const OwnedResourceState* ownedResource = ToConstOwnedResourceState(resource);
	return NativeResourceHandle{ownedResource != nullptr ? ownedResource->Resource.Get() : nullptr};
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = BuildTextureResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = BuildBufferResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiOwnedHeapHandle D3D12RenderHardwareInterface::CreateOwnedHeap(
	RhiTransientAllocationPool pool,
	std::uint64_t sizeInBytes,
	std::uint64_t alignment,
	std::wstring_view debugName)
{
	if (m_rhi == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	auto ownedHeap = std::make_unique<OwnedHeapState>();
	D3D12_HEAP_DESC heapDesc{};
	heapDesc.SizeInBytes = sizeInBytes;
	heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapDesc.Properties.CreationNodeMask = 0;
	heapDesc.Properties.VisibleNodeMask = 0;
	heapDesc.Alignment = alignment;
	heapDesc.Flags = ToHeapFlags(pool);
	if (FAILED(m_rhi->GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(ownedHeap->Heap.ReleaseAndGetAddressOf()))))
	{
		return {};
	}

	ownedHeap->Heap->SetName(CopyDebugName(debugName, L"TransientHeap").c_str());
	return RhiOwnedHeapHandle{ownedHeap.release()};
}

void D3D12RenderHardwareInterface::ReleaseOwnedHeap(RhiOwnedHeapHandle heap) noexcept
{
	delete ToOwnedHeapState(heap);
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreatePlacedTextureResource(
	RhiOwnedHeapHandle heap,
	std::uint64_t heapOffset,
	const RhiTransientTextureAllocationDesc& desc,
	std::wstring_view debugName)
{
	OwnedHeapState* ownedHeap = ToOwnedHeapState(heap);
	if (m_rhi == nullptr || ownedHeap == nullptr || ownedHeap->Heap == nullptr)
	{
		return {};
	}

	auto ownedResource = std::make_unique<OwnedResourceState>();
	const D3D12_RESOURCE_DESC resourceDesc = BuildTextureResourceDesc(desc.ResourceDesc);
	const D3D12_CLEAR_VALUE clearValue = BuildClearValue(desc.ClearValue);
	const D3D12_CLEAR_VALUE* clearValuePtr =
	    desc.ClearValue.ValueType == RhiOptimizedClearValue::Type::None ? nullptr : &clearValue;
	if (FAILED(m_rhi->GetDevice()->CreatePlacedResource(
	        ownedHeap->Heap.Get(),
	        heapOffset,
	        &resourceDesc,
	        ToD3D12ResourceState(desc.InitialState),
	        clearValuePtr,
	        IID_PPV_ARGS(ownedResource->Resource.ReleaseAndGetAddressOf()))))
	{
		return {};
	}

	ownedResource->Resource->SetName(CopyDebugName(debugName, L"PlacedTexture").c_str());
	return RhiOwnedResourceHandle{ownedResource.release()};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreatePlacedBufferResource(
	RhiOwnedHeapHandle heap,
	std::uint64_t heapOffset,
	const RhiTransientBufferAllocationDesc& desc,
	std::wstring_view debugName)
{
	OwnedHeapState* ownedHeap = ToOwnedHeapState(heap);
	if (m_rhi == nullptr || ownedHeap == nullptr || ownedHeap->Heap == nullptr)
	{
		return {};
	}

	auto ownedResource = std::make_unique<OwnedResourceState>();
	const D3D12_RESOURCE_DESC resourceDesc = BuildBufferResourceDesc(desc.ResourceDesc);
	if (FAILED(m_rhi->GetDevice()->CreatePlacedResource(
	        ownedHeap->Heap.Get(),
	        heapOffset,
	        &resourceDesc,
	        ToD3D12ResourceState(desc.InitialState),
	        nullptr,
	        IID_PPV_ARGS(ownedResource->Resource.ReleaseAndGetAddressOf()))))
	{
		return {};
	}

	ownedResource->Resource->SetName(CopyDebugName(debugName, L"PlacedBuffer").c_str());
	return RhiOwnedResourceHandle{ownedResource.release()};
}

void D3D12RenderHardwareInterface::CreateRenderTargetView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_RENDER_TARGET_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	m_rhi->GetDevice()->CreateRenderTargetView(ToD3D12Resource(resource), &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateDepthStencilView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	viewDesc.Flags = D3D12_DSV_FLAG_NONE;
	m_rhi->GetDevice()->CreateDepthStencilView(ToD3D12Resource(resource), &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateTextureShaderResourceView(
	NativeResourceHandle resource,
	PixelFormat format,
	RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDesc.Texture2D.MostDetailedMip = 0;
	viewDesc.Texture2D.MipLevels = 1;
	m_rhi->GetDevice()->CreateShaderResourceView(ToD3D12Resource(resource), &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateTextureUnorderedAccessView(
	NativeResourceHandle resource,
	PixelFormat format,
	RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipSlice = 0;
	viewDesc.Texture2D.PlaneSlice = 0;
	m_rhi->GetDevice()->CreateUnorderedAccessView(ToD3D12Resource(resource), nullptr, &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateBufferShaderResourceView(
	NativeResourceHandle resource,
	std::uint64_t sizeInBytes,
	std::uint32_t strideInBytes,
	RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination || sizeInBytes == 0)
	{
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (strideInBytes > 0)
	{
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		viewDesc.Buffer.StructureByteStride = strideInBytes;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / strideInBytes);
	}
	else
	{
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		viewDesc.Buffer.StructureByteStride = 0;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / sizeof(std::uint32_t));
	}
	m_rhi->GetDevice()->CreateShaderResourceView(ToD3D12Resource(resource), &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateBufferUnorderedAccessView(
	NativeResourceHandle resource,
	std::uint64_t sizeInBytes,
	std::uint32_t strideInBytes,
	RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination || sizeInBytes == 0)
	{
		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	if (strideInBytes > 0)
	{
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		viewDesc.Buffer.StructureByteStride = strideInBytes;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / strideInBytes);
	}
	else
	{
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		viewDesc.Buffer.StructureByteStride = 0;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / sizeof(std::uint32_t));
	}
	m_rhi->GetDevice()->CreateUnorderedAccessView(ToD3D12Resource(resource), nullptr, &viewDesc, ToD3D12CpuDescriptor(destination));
}

bool D3D12RenderHardwareInterface::SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept
{
	return ResourceSupportsUnorderedAccess(ToD3D12Resource(resource));
}

void D3D12RenderHardwareInterface::TransitionResource(
	NativeGraphicsCommandListHandle commandList,
	NativeResourceHandle resource,
	ResourceState before,
	ResourceState after) const noexcept
{
	ID3D12GraphicsCommandList* const nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	ID3D12Resource* const nativeResource = ToD3D12Resource(resource);
	if (nativeCommandList == nullptr || nativeResource == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = nativeResource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = ToD3D12ResourceState(before);
	barrier.Transition.StateAfter = ToD3D12ResourceState(after);
	nativeCommandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderHardwareInterface::BeginPresentRenderPass(NativeGraphicsCommandListHandle commandList, const float clearColor[4])
    const noexcept
{
	if (m_swapChain == nullptr || !commandList)
	{
		return;
	}

	auto* nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	ID3D12Resource* presentTexture = m_swapChain->GetCurrentResource();
	if (nativeCommandList == nullptr || presentTexture == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER transitionToRenderTarget{};
	transitionToRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transitionToRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	transitionToRenderTarget.Transition.pResource = presentTexture;
	transitionToRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	transitionToRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	transitionToRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	nativeCommandList->ResourceBarrier(1, &transitionToRenderTarget);

	ID3D12DescriptorHeap* heaps[2] = {};
	UINT heapCount = 0;
	if (m_descriptorHeapManager != nullptr)
	{
		if (D3D12DescriptorHeap* srvHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
		{
			heaps[heapCount++] = srvHeap->GetRaw();
		}

		if (D3D12DescriptorHeap* samplerHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER))
		{
			heaps[heapCount++] = samplerHeap->GetRaw();
		}
	}

	if (heapCount > 0)
	{
		nativeCommandList->SetDescriptorHeaps(heapCount, heaps);
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_swapChain->GetCPUHandle();
	nativeCommandList->OMSetRenderTargets(1, &renderTargetView, FALSE, nullptr);

	static constexpr float defaultClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	nativeCommandList->ClearRenderTargetView(renderTargetView, clearColor != nullptr ? clearColor : defaultClearColor, 0, nullptr);
}

void D3D12RenderHardwareInterface::EndPresentRenderPass(NativeGraphicsCommandListHandle commandList) const noexcept
{
	if (m_swapChain == nullptr || !commandList)
	{
		return;
	}

	auto* nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	ID3D12Resource* presentTexture = m_swapChain->GetCurrentResource();
	if (nativeCommandList == nullptr || presentTexture == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER transitionToPresent{};
	transitionToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transitionToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	transitionToPresent.Transition.pResource = presentTexture;
	transitionToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	transitionToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	transitionToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	nativeCommandList->ResourceBarrier(1, &transitionToPresent);
}

PixelFormat D3D12RenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferFormat() : PixelFormat::Unknown;
}

void D3D12RenderHardwareInterface::SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept
{
	m_samplerTableHandle = samplerTableHandle;
}

D3D12RenderHardwareInterface::DescriptorTableRecord* D3D12RenderHardwareInterface::FindDescriptorTableRecord(
	RhiDescriptorTableHandle tableHandle) noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

const D3D12RenderHardwareInterface::DescriptorTableRecord* D3D12RenderHardwareInterface::FindDescriptorTableRecord(
	RhiDescriptorTableHandle tableHandle) const noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	const DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableCpuHandle(
	RhiDescriptorTableHandle tableHandle,
	std::uint32_t descriptorIndex) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->descriptorCount)
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE{};
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = record->nativeHandle.GetCPU();
	cpuHandle.ptr += static_cast<SIZE_T>(descriptorIndex) * record->nativeHandle.GetIncrementSize();
	return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableGpuHandle(
	RhiDescriptorTableHandle tableHandle) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	return record != nullptr ? record->nativeHandle.GetGPU() : D3D12_GPU_DESCRIPTOR_HANDLE{};
}

D3D12_DESCRIPTOR_HEAP_TYPE D3D12RenderHardwareInterface::ToNativeDescriptorHeapType(RhiDescriptorHeapType heapType) noexcept
{
	return ToD3D12DescriptorHeapType(heapType);
}