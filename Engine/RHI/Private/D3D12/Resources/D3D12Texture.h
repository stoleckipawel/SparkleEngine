#pragma once

#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "Resources/Texture.h"
#include "Resources/RhiTextureUpload.h"
#include <d3d12.h>
#include <wrl/client.h>

#include <memory>

using Microsoft::WRL::ComPtr;

class D3D12DescriptorHeapManager;
struct D3D12GpuAllocationRecord;
class D3D12Rhi;

class D3D12Texture final : public Texture
{
  public:
	D3D12Texture(D3D12Rhi& rhi, RhiTextureUploadDesc textureUpload, D3D12DescriptorHeapManager& descriptorHeapManager);

	~D3D12Texture() noexcept override;

	D3D12Texture(const D3D12Texture&) = delete;
	D3D12Texture& operator=(const D3D12Texture&) = delete;

	D3D12Texture(D3D12Texture&&) = delete;
	D3D12Texture& operator=(D3D12Texture&&) = delete;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return m_srvHandle.GetCPU(); }

	const ComPtr<ID3D12Resource2>& GetResource() const noexcept { return m_textureResource; }

	void WriteShaderResourceView(RhiCpuDescriptorHandle destination) const override;
	TextureRuntimeInfo GetRuntimeInfo() const noexcept override;

  private:
	void CreateResource();

	void UploadToGPU();

	void CreateShaderResourceView();
	D3D12_SHADER_RESOURCE_VIEW_DESC BuildShaderResourceViewDesc() const noexcept;

	D3D12Rhi& m_rhi;
	ComPtr<ID3D12Resource2> m_textureResource;
	ComPtr<ID3D12Resource2> m_uploadResource;
	std::unique_ptr<D3D12GpuAllocationRecord> m_textureAllocation;
	std::unique_ptr<D3D12GpuAllocationRecord> m_uploadAllocation;
	RhiTextureUploadDesc m_textureUpload;
	D3D12DescriptorHandle m_srvHandle;
	D3D12_RESOURCE_DESC m_texResourceDesc = {};
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
};
