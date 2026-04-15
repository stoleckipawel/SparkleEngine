#pragma once

#include "../../RHIAPI.h"
#include "../../Interop/RenderHardwareInterface.h"

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <vector>

enum class D3D12BindingOverrideType : std::uint8_t
{
	ConstantBufferView,
	ShaderResourceView,
	UnorderedAccessView,
	DescriptorTable,
	RootConstants,
};

struct D3D12BindingOverride
{
	std::string Name;
	D3D12BindingOverrideType Type = D3D12BindingOverrideType::DescriptorTable;
	D3D12_GPU_VIRTUAL_ADDRESS GpuAddress = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTable = {};
	const void* ConstantsData = nullptr;
	std::uint32_t ConstantCount = 0;
};

class SPARKLE_RHI_API D3D12PassBindingOverrides final
{
  public:
	void SetConstantBufferView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
	void SetShaderResourceView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
	void SetUnorderedAccessView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
	void SetDescriptorTable(const char* name, D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable);
	void SetDescriptorTable(const char* name, RhiGpuDescriptorHandle descriptorTable);
	void SetRootConstants(const char* name, const void* data, std::uint32_t constantCount);

	const D3D12BindingOverride* Find(const char* name, D3D12BindingOverrideType type) const noexcept;

  private:
	std::vector<D3D12BindingOverride> m_overrides;
};