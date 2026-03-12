#include "PCH.h"
#include "D3D12RootSignature.h"
#include "D3D12Rhi.h"
#include "D3D12RootBindings.h"
#include "DebugUtils.h"
#include "Log.h"

D3D12RootSignature::D3D12RootSignature(D3D12Rhi& rhi) : m_rhi(rhi)
{
	Create();
}

void D3D12RootSignature::Create()
{
	CD3DX12_ROOT_PARAMETER rootParameters[RootBindings::RootParam::Count] = {};

	CD3DX12_DESCRIPTOR_RANGE srvRange = {};
	srvRange.Init(
	    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	    RootBindings::SRVRegister::MaterialTextureCount,
	    RootBindings::SRVRegister::MaterialTableBase);

	CD3DX12_DESCRIPTOR_RANGE samplerRange = {};

	samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, RootBindings::SamplerRegister::Count, 0);

	rootParameters[RootBindings::RootParam::PerFrame].InitAsConstantBufferView(
	    RootBindings::CBRegister::PerFrame,
	    0,
	    RootBindings::Visibility::PerFrame);

	rootParameters[RootBindings::RootParam::PerView].InitAsConstantBufferView(
	    RootBindings::CBRegister::PerView,
	    0,
	    RootBindings::Visibility::PerView);

	rootParameters[RootBindings::RootParam::PerObjectVS].InitAsConstantBufferView(
	    RootBindings::CBRegister::PerObjectVS,
	    0,
	    RootBindings::Visibility::PerObjectVS);

	rootParameters[RootBindings::RootParam::PerObjectPS].InitAsConstantBufferView(
	    RootBindings::CBRegister::PerObjectPS,
	    0,
	    RootBindings::Visibility::PerObjectPS);

	rootParameters[RootBindings::RootParam::TextureSRV].InitAsDescriptorTable(1, &srvRange, RootBindings::Visibility::TextureSRV);

	rootParameters[RootBindings::RootParam::SamplerTable].InitAsDescriptorTable(1, &samplerRange, RootBindings::Visibility::SamplerTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc
	    .Init(RootBindings::RootParam::Count, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	CHECK(m_rhi.GetDevice()->CreateRootSignature(
	    0,
	    signature->GetBufferPointer(),
	    signature->GetBufferSize(),
	    IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
	DebugUtils::SetDebugName(m_rootSignature, L"RHI_RootSignature");
}

D3D12RootSignature::~D3D12RootSignature() noexcept
{
	m_rootSignature.Reset();
}