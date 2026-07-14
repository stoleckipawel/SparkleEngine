#pragma once

#include "Formats/CompareOp.h"
#include "Formats/PixelFormat.h"
#include "Device/RenderHardwareInterface.h"

#include <d3d12.h>
#include <dxgi1_6.h>

class D3D12TypeConversions final
{
  public:
	static DXGI_FORMAT ToDxgiFormat(PixelFormat format) noexcept;
	static D3D12_COMPARISON_FUNC ToComparisonFunc(CompareOp compareOp) noexcept;
	static ID3D12GraphicsCommandList* ToGraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept;
	static ID3D12Resource* ToResource(NativeResourceHandle handle) noexcept;
	static D3D12_CPU_DESCRIPTOR_HANDLE ToCpuDescriptor(RhiCpuDescriptorHandle handle) noexcept;
	static D3D12_GPU_DESCRIPTOR_HANDLE ToGpuDescriptor(RhiGpuDescriptorHandle handle) noexcept;
	static D3D12_DESCRIPTOR_HEAP_TYPE ToDescriptorHeapType(ERhiDescriptorAllocatorType descriptorType) noexcept;
	static D3D12_RESOURCE_STATES ToResourceStates(ResourceState state) noexcept;
	static D3D12_PRIMITIVE_TOPOLOGY ToPrimitiveTopology(RhiPrimitiveTopology topology) noexcept;
	static DXGI_FORMAT ToIndexFormat(RhiIndexFormat format) noexcept;
	static D3D12_RESOURCE_DESC BuildTextureResourceDesc(const RhiTextureResourceDesc& desc) noexcept;
	static D3D12_RESOURCE_DESC BuildBufferResourceDesc(const RhiBufferResourceDesc& desc) noexcept;
	static D3D12_CLEAR_VALUE BuildClearValue(const RhiOptimizedClearValue& clearValue) noexcept;

  private:
	D3D12TypeConversions() = delete;
	~D3D12TypeConversions() = delete;
};
