#include "PCH.h"
#include "D3D12/Pipeline/D3D12RootSignatureBuilder.h"
#include "D3D12/Pipeline/D3D12RootSignature.h"

std::uint32_t D3D12RootSignatureBuilder::AddConstantBufferView(
    std::uint32_t shaderRegister,
    std::uint32_t registerSpace,
    D3D12_SHADER_VISIBILITY visibility)
{
	const auto index = static_cast<std::uint32_t>(m_entries.size());
	m_entries.push_back({ParamKind::CBV, shaderRegister, registerSpace, visibility, {}, 0});
	return index;
}

std::uint32_t D3D12RootSignatureBuilder::AddShaderResourceView(
    std::uint32_t shaderRegister,
    std::uint32_t registerSpace,
    D3D12_SHADER_VISIBILITY visibility)
{
	const auto index = static_cast<std::uint32_t>(m_entries.size());
	m_entries.push_back({ParamKind::SRV, shaderRegister, registerSpace, visibility, {}, 0});
	return index;
}

std::uint32_t D3D12RootSignatureBuilder::AddUnorderedAccessView(
    std::uint32_t shaderRegister,
    std::uint32_t registerSpace,
    D3D12_SHADER_VISIBILITY visibility)
{
	const auto index = static_cast<std::uint32_t>(m_entries.size());
	m_entries.push_back({ParamKind::UAV, shaderRegister, registerSpace, visibility, {}, 0});
	return index;
}

std::uint32_t D3D12RootSignatureBuilder::AddDescriptorTable(
    D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
    std::uint32_t descriptorCount,
    std::uint32_t baseShaderRegister,
    D3D12_SHADER_VISIBILITY visibility)
{
	const auto index = static_cast<std::uint32_t>(m_entries.size());
	m_entries.push_back({ParamKind::DescriptorTable, baseShaderRegister, 0, visibility, rangeType, descriptorCount});
	return index;
}

std::uint32_t D3D12RootSignatureBuilder::AddRootConstants(
    std::uint32_t num32BitValues,
    std::uint32_t shaderRegister,
    std::uint32_t registerSpace,
    D3D12_SHADER_VISIBILITY visibility)
{
	const auto index = static_cast<std::uint32_t>(m_entries.size());
	m_entries.push_back({ParamKind::Constants, shaderRegister, registerSpace, visibility, {}, num32BitValues});
	return index;
}

void D3D12RootSignatureBuilder::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	m_flags = flags;
}

std::unique_ptr<D3D12RootSignature> D3D12RootSignatureBuilder::Build(D3D12Rhi& rhi, const wchar_t* debugName) const
{
	std::vector<CD3DX12_DESCRIPTOR_RANGE> ranges;
	ranges.reserve(m_entries.size());

	std::vector<CD3DX12_ROOT_PARAMETER> params(m_entries.size());

	for (std::size_t i = 0; i < m_entries.size(); ++i)
	{
		const auto& entry = m_entries[i];
		switch (entry.Kind)
		{
			case ParamKind::CBV:
				params[i].InitAsConstantBufferView(entry.ShaderRegister, entry.RegisterSpace, entry.Visibility);
				break;
			case ParamKind::SRV:
				params[i].InitAsShaderResourceView(entry.ShaderRegister, entry.RegisterSpace, entry.Visibility);
				break;
			case ParamKind::UAV:
				params[i].InitAsUnorderedAccessView(entry.ShaderRegister, entry.RegisterSpace, entry.Visibility);
				break;
			case ParamKind::DescriptorTable:
			{
				auto& range = ranges.emplace_back();
				range.Init(entry.RangeType, entry.Count, entry.ShaderRegister);
				params[i].InitAsDescriptorTable(1, &range, entry.Visibility);
				break;
			}
			case ParamKind::Constants:
				params[i].InitAsConstants(entry.Count, entry.ShaderRegister, entry.RegisterSpace, entry.Visibility);
				break;
		}
	}

	RootSignatureDesc desc{};
	desc.Parameters = params.data();
	desc.ParameterCount = static_cast<std::uint32_t>(params.size());
	desc.Flags = m_flags;
	desc.DebugName = debugName;

	return std::make_unique<D3D12RootSignature>(rhi, desc);
}
