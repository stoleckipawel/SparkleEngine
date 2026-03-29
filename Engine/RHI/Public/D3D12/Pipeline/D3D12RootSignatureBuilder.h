#pragma once

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <vector>

class D3D12Rhi;
class D3D12RootSignature;

class D3D12RootSignatureBuilder final
{
  public:
	std::uint32_t AddConstantBufferView(std::uint32_t shaderRegister, std::uint32_t registerSpace, D3D12_SHADER_VISIBILITY visibility);

	std::uint32_t AddShaderResourceView(std::uint32_t shaderRegister, std::uint32_t registerSpace, D3D12_SHADER_VISIBILITY visibility);

	std::uint32_t AddUnorderedAccessView(std::uint32_t shaderRegister, std::uint32_t registerSpace, D3D12_SHADER_VISIBILITY visibility);

	std::uint32_t AddDescriptorTable(
	    D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
	    std::uint32_t descriptorCount,
	    std::uint32_t baseShaderRegister,
	    D3D12_SHADER_VISIBILITY visibility);

	std::uint32_t AddRootConstants(
	    std::uint32_t num32BitValues,
	    std::uint32_t shaderRegister,
	    std::uint32_t registerSpace,
	    D3D12_SHADER_VISIBILITY visibility);

	void SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);

	std::unique_ptr<D3D12RootSignature> Build(D3D12Rhi& rhi, const wchar_t* debugName) const;

  private:
	enum class ParamKind : std::uint8_t
	{
		CBV,
		SRV,
		UAV,
		DescriptorTable,
		Constants
	};

	struct Entry
	{
		ParamKind Kind;
		std::uint32_t ShaderRegister = 0;
		std::uint32_t RegisterSpace = 0;
		D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL;
		D3D12_DESCRIPTOR_RANGE_TYPE RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		std::uint32_t Count = 0;
	};

	std::vector<Entry> m_entries;
	D3D12_ROOT_SIGNATURE_FLAGS m_flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
};
