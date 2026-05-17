#include "PCH.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/D3D12TypeConversions.h"

static const auto g_samplerLibraryLogger = Logging::GetOrCreateLogger("RHI.D3D12.Samplers");

bool D3D12SamplerLibrary::TryGetSlot(const RhiSamplerDesc& samplerDesc, Slot& outSlot) noexcept
{
	RhiSamplerAddressMode addressMode = RhiSamplerAddressMode::Wrap;
	if (!TryGetUniformAddressMode(samplerDesc, addressMode))
	{
		return false;
	}

	if (samplerDesc.MaxAnisotropy != RhiSamplerAnisotropy::X1)
	{
		const bool anisotropyApplies =
		    samplerDesc.MinMagFilter == RhiSamplerMinMagFilter::Linear && samplerDesc.MipFilter == RhiSamplerMipFilter::Linear;
		if (anisotropyApplies)
		{
			return TryGetAnisotropicSlot(samplerDesc.MaxAnisotropy, addressMode, outSlot);
		}
	}

	switch (samplerDesc.MinMagFilter)
	{
		case RhiSamplerMinMagFilter::Point:
			return TryGetPointSlot(samplerDesc.MipFilter, addressMode, outSlot);
		case RhiSamplerMinMagFilter::Linear:
			return TryGetLinearSlot(samplerDesc.MipFilter, addressMode, outSlot);
		default:
			return false;
	}
}

D3D12SamplerLibrary::D3D12SamplerLibrary(D3D12Rhi& rhi, RenderHardwareInterface& renderHardwareInterface) :
    m_rhi(&rhi), m_renderHardwareInterface(&renderHardwareInterface)
{
	constexpr uint32_t samplerCount = static_cast<uint32_t>(Slot::Count);

	m_tableHandle = m_renderHardwareInterface->AllocateDescriptorTable(ERhiDescriptorAllocatorType::Sampler, samplerCount);
	if (!m_tableHandle)
	{
		Diagnostics::Fail(g_samplerLibraryLogger, __FILE__, __LINE__, "Failed to allocate sampler descriptor table.");
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
	if (m_tableHandle && m_renderHardwareInterface)
	{
		m_renderHardwareInterface->ReleaseDescriptorTable(m_tableHandle);
		m_tableHandle = {};
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
	desc.MaxLOD = config.mip == MipFilter::None ? 0.0f : D3D12_FLOAT32_MAX;

	const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = D3D12TypeConversions::ToCpuDescriptor(
	    m_renderHardwareInterface->GetDescriptorTableCpuHandle(m_tableHandle, static_cast<uint32_t>(slot)));

	m_rhi->GetDevice()->CreateSampler(&desc, cpuHandle);
}

bool D3D12SamplerLibrary::TryGetAddressOffset(RhiSamplerAddressMode addressMode, std::uint32_t& outOffset) noexcept
{
	switch (addressMode)
	{
		case RhiSamplerAddressMode::Wrap:
			outOffset = 0;
			return true;
		case RhiSamplerAddressMode::Clamp:
			outOffset = 1;
			return true;
		case RhiSamplerAddressMode::Mirror:
			outOffset = 2;
			return true;
		default:
			return false;
	}
}

bool D3D12SamplerLibrary::TryGetUniformAddressMode(const RhiSamplerDesc& samplerDesc, RhiSamplerAddressMode& outAddressMode) noexcept
{
	if (samplerDesc.Address.U != samplerDesc.Address.V || samplerDesc.Address.U != samplerDesc.Address.W)
	{
		return false;
	}

	outAddressMode = samplerDesc.Address.U;
	return true;
}

bool D3D12SamplerLibrary::TryGetPointSlot(RhiSamplerMipFilter mipFilter, RhiSamplerAddressMode addressMode, Slot& outSlot) noexcept
{
	std::uint32_t addressOffset = 0;
	if (!TryGetAddressOffset(addressMode, addressOffset))
	{
		return false;
	}

	Slot baseSlot = Slot::Count;
	switch (mipFilter)
	{
		case RhiSamplerMipFilter::None:
			baseSlot = Slot::PointNoMipWrap;
			break;
		case RhiSamplerMipFilter::Point:
			baseSlot = Slot::PointMipPointWrap;
			break;
		case RhiSamplerMipFilter::Linear:
			baseSlot = Slot::PointMipLinearWrap;
			break;
		default:
			return false;
	}

	outSlot = static_cast<Slot>(static_cast<std::uint32_t>(baseSlot) + addressOffset);
	return true;
}

bool D3D12SamplerLibrary::TryGetLinearSlot(RhiSamplerMipFilter mipFilter, RhiSamplerAddressMode addressMode, Slot& outSlot) noexcept
{
	std::uint32_t addressOffset = 0;
	if (!TryGetAddressOffset(addressMode, addressOffset))
	{
		return false;
	}

	Slot baseSlot = Slot::Count;
	switch (mipFilter)
	{
		case RhiSamplerMipFilter::None:
			baseSlot = Slot::LinearNoMipWrap;
			break;
		case RhiSamplerMipFilter::Point:
			baseSlot = Slot::LinearMipPointWrap;
			break;
		case RhiSamplerMipFilter::Linear:
			baseSlot = Slot::LinearMipLinearWrap;
			break;
		default:
			return false;
	}

	outSlot = static_cast<Slot>(static_cast<std::uint32_t>(baseSlot) + addressOffset);
	return true;
}

bool D3D12SamplerLibrary::TryGetAnisotropicSlot(
    RhiSamplerAnisotropy maxAnisotropy,
    RhiSamplerAddressMode addressMode,
    Slot& outSlot) noexcept
{
	std::uint32_t addressOffset = 0;
	if (!TryGetAddressOffset(addressMode, addressOffset))
	{
		return false;
	}

	Slot baseSlot = Slot::Count;
	switch (maxAnisotropy)
	{
		case RhiSamplerAnisotropy::X1:
			baseSlot = Slot::Aniso1xWrap;
			break;
		case RhiSamplerAnisotropy::X2:
			baseSlot = Slot::Aniso2xWrap;
			break;
		case RhiSamplerAnisotropy::X4:
			baseSlot = Slot::Aniso4xWrap;
			break;
		case RhiSamplerAnisotropy::X8:
			baseSlot = Slot::Aniso8xWrap;
			break;
		case RhiSamplerAnisotropy::X16:
			baseSlot = Slot::Aniso16xWrap;
			break;
		default:
			return false;
	}

	outSlot = static_cast<Slot>(static_cast<std::uint32_t>(baseSlot) + addressOffset);
	return true;
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
