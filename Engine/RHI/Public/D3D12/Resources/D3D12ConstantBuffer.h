// ============================================================================
// D3D12ConstantBuffer.h
// ----------------------------------------------------------------------------
// Template class for managing typed GPU constant buffers.
//
// USAGE:
//   D3D12ConstantBuffer<PerFrameData> frameCB(descriptorHeapManager);
//   frameCB.Update(frameData);
//   cmdList->SetGraphicsRootConstantBufferView(0, frameCB.GetGPUVirtualAddress());
//
// DESIGN:
//   - Creates persistently-mapped upload buffer (256-byte aligned)
//   - Allocates CBV descriptor from heap manager
//   - Supports both root CBV and descriptor table binding
//
// BINDING METHODS:
//   - GetGPUVirtualAddress(): For SetGraphicsRootConstantBufferView (preferred)
//   - GetGPUHandle(): For descriptor table binding
//
// NOTES:
//   - Non-copyable/non-movable (owns GPU resource and descriptor)
//   - Update() writes directly to mapped memory
// ============================================================================

#pragma once

#include "D3D12DescriptorHeap.h"
#include "D3D12DescriptorHeapManager.h"
#include "DebugUtils.h"
#include <cstring>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Rhi;

template <typename T> class D3D12ConstantBuffer
{
  public:
	// Create and map constant buffer, create a CBV view. Allocates a descriptor via the manager.
	explicit D3D12ConstantBuffer(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) :
	    m_rhi(&rhi),
	    m_descriptorHeapManager(&descriptorHeapManager),
	    m_cbvHandle(descriptorHeapManager.AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
	    m_constantBufferSize((sizeof(T) + 255) & ~255)
	{
		std::memset(&m_constantBufferData, 0, sizeof(T));
		CreateResource();
		CreateConstantBufferView();
	}

	// Updates the buffer with new data
	void Update(const T& data) noexcept
	{
		m_constantBufferData = data;
		if (m_mappedData)
		{
			std::memcpy(m_mappedData, &m_constantBufferData, sizeof(T));
		}
	}

	// Returns the GPU virtual address for root CBV binding (SetGraphicsRootConstantBufferView)
	// This is the preferred binding method for frequently-updated constant buffers.
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const noexcept
	{
		return m_resource ? m_resource->GetGPUVirtualAddress() : 0;
	}

	// Returns the GPU descriptor handle for descriptor table binding
	// Use this only when binding via descriptor tables, not for root CBVs.
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_cbvHandle.GetGPU(); }

	// Returns the CPU descriptor handle for descriptor heap management
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return m_cbvHandle.GetCPU(); }

	// Returns the aligned size of the backing constant buffer in bytes (256-byte aligned)
	UINT GetSizeInBytes() const noexcept { return m_constantBufferSize; }

	// Returns true if the buffer resource is valid and mapped
	bool IsValid() const noexcept { return m_resource != nullptr && m_mappedData != nullptr; }

	// No copy or move allowed, strict ownership
	D3D12ConstantBuffer(const D3D12ConstantBuffer&) = delete;
	D3D12ConstantBuffer& operator=(const D3D12ConstantBuffer&) = delete;
	D3D12ConstantBuffer(D3D12ConstantBuffer&&) = delete;
	D3D12ConstantBuffer& operator=(D3D12ConstantBuffer&&) = delete;

	~D3D12ConstantBuffer() noexcept
	{
		if (m_resource)
		{
			m_resource->Unmap(0, nullptr);
			m_resource.Reset();
		}
		m_mappedData = nullptr;

		if (m_cbvHandle.IsValid())
		{
			m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_cbvHandle);
		}
	}

  private:
	// Create the committed resource and map for CPU writes
	void CreateResource()
	{
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = m_constantBufferSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		CHECK(m_rhi->GetDevice()->CreateCommittedResource(
		    &heapProperties,
		    D3D12_HEAP_FLAG_NONE,
		    &resourceDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ,
		    nullptr,
		    IID_PPV_ARGS(&m_resource)));

		DebugUtils::SetDebugName(m_resource, L"RHI_ConstantBuffer");

		// Map the resource for CPU writes
		D3D12_RANGE readRange = {0, 0};
		void* mapped = nullptr;

		CHECK(m_resource->Map(0, &readRange, &mapped));
		m_mappedData = mapped;
	}

	// Creates a constant buffer view at the given CPU descriptor handle
	void CreateConstantBufferView()
	{
		m_constantBufferViewDesc.BufferLocation = m_resource->GetGPUVirtualAddress();
		m_constantBufferViewDesc.SizeInBytes = m_constantBufferSize;
		m_rhi->GetDevice()->CreateConstantBufferView(&m_constantBufferViewDesc, GetCPUHandle());
	}

  private:
	D3D12Rhi* m_rhi = nullptr;                                      // RHI reference
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;  // Descriptor heap manager reference
	ComPtr<ID3D12Resource2> m_resource = nullptr;                   // GPU resource
	D3D12DescriptorHandle m_cbvHandle;                              // CBV descriptor handle
	T m_constantBufferData;                                         // Cached buffer data
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_constantBufferViewDesc = {};
	void* m_mappedData = nullptr;  // Pointer to mapped memory
	UINT m_constantBufferSize;     // Aligned buffer size (256 bytes)
};
