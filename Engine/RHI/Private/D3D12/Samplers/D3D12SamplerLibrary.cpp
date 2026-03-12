#include "PCH.h"
#include "D3D12SamplerLibrary.h"
#include "D3D12DescriptorHeapManager.h"
#include "D3D12Rhi.h"

D3D12SamplerLibrary::D3D12SamplerLibrary(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) :
    m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager)
{
	constexpr uint32_t samplerCount = static_cast<uint32_t>(Slot::Count);

	m_tableHandle = m_descriptorHeapManager->AllocateContiguous(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerCount);
	if (!m_tableHandle.IsValid())
	{
		LOG_FATAL("Failed to allocate sampler descriptor table.");
		return;
	}

	m_descriptorSize = m_rhi->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	CreateSampler(Slot::PointMipPointWrap, {MinMagFilter::Point, MipFilter::Point, AddressMode::Wrap, 1});
	CreateSampler(Slot::PointMipPointClamp, {MinMagFilter::Point, MipFilter::Point, AddressMode::Clamp, 1});
	CreateSampler(Slot::PointMipPointMirror, {MinMagFilter::Point, MipFilter::Point, AddressMode::Mirror, 1});
	CreateSampler(Slot::PointMipLinearWrap, {MinMagFilter::Point, MipFilter::Linear, AddressMode::Wrap, 1});
	CreateSampler(Slot::PointMipLinearClamp, {MinMagFilter::Point, MipFilter::Linear, AddressMode::Clamp, 1});
	CreateSampler(Slot::PointMipLinearMirror, {MinMagFilter::Point, MipFilter::Linear, AddressMode::Mirror, 1});
	CreateSampler(Slot::PointNoMipWrap, {MinMagFilter::Point, MipFilter::None, AddressMode::Wrap, 1});
	CreateSampler(Slot::PointNoMipClamp, {MinMagFilter::Point, MipFilter::None, AddressMode::Clamp, 1});
	CreateSampler(Slot::PointNoMipMirror, {MinMagFilter::Point, MipFilter::None, AddressMode::Mirror, 1});

	CreateSampler(Slot::LinearMipPointWrap, {MinMagFilter::Linear, MipFilter::Point, AddressMode::Wrap, 1});
	CreateSampler(Slot::LinearMipPointClamp, {MinMagFilter::Linear, MipFilter::Point, AddressMode::Clamp, 1});
	CreateSampler(Slot::LinearMipPointMirror, {MinMagFilter::Linear, MipFilter::Point, AddressMode::Mirror, 1});
	CreateSampler(Slot::LinearMipLinearWrap, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Wrap, 1});
	CreateSampler(Slot::LinearMipLinearClamp, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Clamp, 1});
	CreateSampler(Slot::LinearMipLinearMirror, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Mirror, 1});
	CreateSampler(Slot::LinearNoMipWrap, {MinMagFilter::Linear, MipFilter::None, AddressMode::Wrap, 1});
	CreateSampler(Slot::LinearNoMipClamp, {MinMagFilter::Linear, MipFilter::None, AddressMode::Clamp, 1});
	CreateSampler(Slot::LinearNoMipMirror, {MinMagFilter::Linear, MipFilter::None, AddressMode::Mirror, 1});

	CreateSampler(Slot::Aniso1xWrap, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Wrap, 1});
	CreateSampler(Slot::Aniso1xClamp, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Clamp, 1});
	CreateSampler(Slot::Aniso1xMirror, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Mirror, 1});
	CreateSampler(Slot::Aniso2xWrap, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Wrap, 2});
	CreateSampler(Slot::Aniso2xClamp, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Clamp, 2});
	CreateSampler(Slot::Aniso2xMirror, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Mirror, 2});
	CreateSampler(Slot::Aniso4xWrap, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Wrap, 4});
	CreateSampler(Slot::Aniso4xClamp, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Clamp, 4});
	CreateSampler(Slot::Aniso4xMirror, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Mirror, 4});
	CreateSampler(Slot::Aniso8xWrap, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Wrap, 8});
	CreateSampler(Slot::Aniso8xClamp, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Clamp, 8});
	CreateSampler(Slot::Aniso8xMirror, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Mirror, 8});
	CreateSampler(Slot::Aniso16xWrap, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Wrap, 16});
	CreateSampler(Slot::Aniso16xClamp, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Clamp, 16});
	CreateSampler(Slot::Aniso16xMirror, {MinMagFilter::Linear, MipFilter::Linear, AddressMode::Mirror, 16});

	m_bInitialized = true;
}

D3D12SamplerLibrary::~D3D12SamplerLibrary() noexcept
{
	if (m_tableHandle.IsValid() && m_descriptorHeapManager)
	{
		m_descriptorHeapManager->FreeContiguous(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_tableHandle, static_cast<uint32_t>(Slot::Count));
		m_tableHandle = D3D12DescriptorHandle{};
	}
	m_bInitialized = false;
}

void D3D12SamplerLibrary::CreateSampler(Slot slot, const SamplerConfig& config)
{
	const bool isAnisotropic = config.maxAnisotropy > 1;

	D3D12_SAMPLER_DESC desc = {};
	desc.Filter = ToD3D12Filter(config.minMag, config.mip, isAnisotropic);
	desc.AddressU = ToD3D12Address(config.address);
	desc.AddressV = desc.AddressU;
	desc.AddressW = desc.AddressU;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = config.maxAnisotropy;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	desc.MinLOD = 0.0f;
	desc.MaxLOD = (config.mip == MipFilter::None) ? 0.0f : D3D12_FLOAT32_MAX;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_tableHandle.GetCPU();
	cpuHandle.ptr += static_cast<SIZE_T>(static_cast<uint32_t>(slot)) * m_descriptorSize;

	m_rhi->GetDevice()->CreateSampler(&desc, cpuHandle);
}

D3D12_FILTER D3D12SamplerLibrary::ToD3D12Filter(MinMagFilter minMag, MipFilter mip, bool anisotropic)
{
	if (anisotropic)
	{
		return D3D12_FILTER_ANISOTROPIC;
	}

	const uint32_t minMagBit = (minMag == MinMagFilter::Linear) ? 1u : 0u;
	const uint32_t mipBit = (mip == MipFilter::Linear) ? 1u : 0u;

	const uint32_t filterValue = (minMagBit << 4) | (minMagBit << 2) | mipBit;

	return static_cast<D3D12_FILTER>(filterValue);
}

D3D12_TEXTURE_ADDRESS_MODE D3D12SamplerLibrary::ToD3D12Address(AddressMode address)
{
	switch (address)
	{
		case AddressMode::Wrap:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case AddressMode::Clamp:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case AddressMode::Mirror:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	}
	return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}
