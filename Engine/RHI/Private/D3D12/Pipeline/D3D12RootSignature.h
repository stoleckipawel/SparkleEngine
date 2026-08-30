#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Rhi;

struct RootSignatureDesc
{
	const D3D12_ROOT_PARAMETER* Parameters = nullptr;
	std::uint32_t ParameterCount = 0;
	const D3D12_STATIC_SAMPLER_DESC* StaticSamplers = nullptr;
	std::uint32_t StaticSamplerCount = 0;
	D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	const wchar_t* DebugName = L"RHI_RootSignature";
};

class D3D12RootSignature
{
public:
	D3D12RootSignature(D3D12Rhi& rhi, const RootSignatureDesc& desc);
	~D3D12RootSignature() noexcept;
	D3D12RootSignature(const D3D12RootSignature&) = delete;
	D3D12RootSignature& operator=(const D3D12RootSignature&) = delete;

	ComPtr<ID3D12RootSignature> Get() noexcept { return m_rootSignature; }
	ID3D12RootSignature* GetRaw() const noexcept { return m_rootSignature.Get(); }

private:
	void Create(const RootSignatureDesc& desc);
	D3D12Rhi& m_rhi;
	ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
};
