#pragma once

#include "TexturePayload.h"
#include "D3D12DescriptorHandle.h"
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12DescriptorHeapManager;
class D3D12Rhi;

class D3D12Texture
{
  public:
	D3D12Texture(D3D12Rhi& rhi, TexturePayload texturePayload, D3D12DescriptorHeapManager& descriptorHeapManager);

	~D3D12Texture() noexcept;

	D3D12Texture(const D3D12Texture&) = delete;
	D3D12Texture& operator=(const D3D12Texture&) = delete;

	D3D12Texture(D3D12Texture&&) = delete;
	D3D12Texture& operator=(D3D12Texture&&) = delete;

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_srvHandle.GetGPU(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return m_srvHandle.GetCPU(); }

	void WriteShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE destination) const;

  private:
	void CreateResource();

	void UploadToGPU();

	void CreateShaderResourceView();
	D3D12_SHADER_RESOURCE_VIEW_DESC BuildShaderResourceViewDesc() const noexcept;

	D3D12Rhi& m_rhi;
	ComPtr<ID3D12Resource2> m_textureResource;
	ComPtr<ID3D12Resource2> m_uploadResource;
	TexturePayload m_texturePayload;
	D3D12DescriptorHandle m_srvHandle;
	D3D12_RESOURCE_DESC m_texResourceDesc = {};
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
};
