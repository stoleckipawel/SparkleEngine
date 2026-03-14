#pragma once

#include "D3D12DescriptorHeap.h"
#include "D3D12DescriptorHeapManager.h"
#include <cstring>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Rhi;

template <typename T> class D3D12ConstantBuffer
{
  public:
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

	void Update(const T& data) noexcept
	{
		m_constantBufferData = data;
		if (m_mappedData)
		{
			std::memcpy(m_mappedData, &m_constantBufferData, sizeof(T));
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const noexcept { return m_resource ? m_resource->GetGPUVirtualAddress() : 0; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_cbvHandle.GetGPU(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return m_cbvHandle.GetCPU(); }

	UINT GetSizeInBytes() const noexcept { return m_constantBufferSize; }

	bool IsValid() const noexcept { return m_resource != nullptr && m_mappedData != nullptr; }

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

		m_resource->SetName(L"RHI_ConstantBuffer");

		D3D12_RANGE readRange = {0, 0};
		void* mapped = nullptr;

		CHECK(m_resource->Map(0, &readRange, &mapped));
		m_mappedData = mapped;
	}

	void CreateConstantBufferView()
	{
		m_constantBufferViewDesc.BufferLocation = m_resource->GetGPUVirtualAddress();
		m_constantBufferViewDesc.SizeInBytes = m_constantBufferSize;
		m_rhi->GetDevice()->CreateConstantBufferView(&m_constantBufferViewDesc, GetCPUHandle());
	}

  private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	ComPtr<ID3D12Resource2> m_resource = nullptr;
	D3D12DescriptorHandle m_cbvHandle;
	T m_constantBufferData;
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_constantBufferViewDesc = {};
	void* m_mappedData = nullptr;
	UINT m_constantBufferSize;
};
