#include "PCH.h"
#include "D3D12RootSignature.h"
#include "D3D12Rhi.h"
#include "Log.h"

D3D12RootSignature::D3D12RootSignature(D3D12Rhi& rhi, const RootSignatureDesc& desc) : m_rhi(rhi)
{
	Create(desc);
}

void D3D12RootSignature::Create(const RootSignatureDesc& desc)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(desc.ParameterCount, desc.Parameters, desc.StaticSamplerCount, desc.StaticSamplers, desc.Flags);

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	CHECK(m_rhi.GetDevice()->CreateRootSignature(
	    0,
	    signature->GetBufferPointer(),
	    signature->GetBufferSize(),
	    IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
	m_rootSignature->SetName(desc.DebugName);
}

D3D12RootSignature::~D3D12RootSignature() noexcept
{
	m_rootSignature.Reset();
}