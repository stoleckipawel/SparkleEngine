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

D3D12SamplerLibrary::D3D12SamplerLibrary(D3D12Rhi& rhi, RhiDescriptorService& descriptorService) :
    m_rhi(&rhi),
    m_descriptorService(&descriptorService)
{
	constexpr uint32_t samplerCount = static_cast<uint32_t>(Slot::Count);

	m_tableHandle = m_descriptorService->AllocateDescriptorTable(ERhiDescriptorAllocatorType::Sampler, samplerCount);
	if (!m_tableHandle)
	{
		Diagnostics::Fatal(g_samplerLibraryLogger, __FILE__, __LINE__, "Failed to allocate sampler descriptor table.");
		return;
	}

	m_descriptorSize = m_rhi->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	const auto createSampler = [this](
	                               Slot slot,
	                               RhiSamplerMinMagFilter minMagFilter,
	                               RhiSamplerMipFilter mipFilter,
	                               RhiSamplerAddressMode addressMode,
	                               RhiSamplerAnisotropy maxAnisotropy)
	{
		CreateSampler(slot, MakeSamplerDesc(minMagFilter, mipFilter, addressMode, maxAnisotropy));
	};

	createSampler(
	    Slot::PointMipPointWrap,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::Point,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::PointMipPointClamp,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::Point,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::PointMipPointMirror,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::Point,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::PointMipLinearWrap,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::PointMipLinearClamp,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::PointMipLinearMirror,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::PointNoMipWrap,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::None,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::PointNoMipClamp,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::None,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::PointNoMipMirror,
	    RhiSamplerMinMagFilter::Point,
	    RhiSamplerMipFilter::None,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X1);

	createSampler(
	    Slot::LinearMipPointWrap,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Point,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::LinearMipPointClamp,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Point,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::LinearMipPointMirror,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Point,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::LinearMipLinearWrap,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::LinearMipLinearClamp,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::LinearMipLinearMirror,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::LinearNoMipWrap,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::None,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::LinearNoMipClamp,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::None,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::LinearNoMipMirror,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::None,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X1);

	createSampler(
	    Slot::Aniso1xWrap,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::Aniso1xClamp,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::Aniso1xMirror,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X1);
	createSampler(
	    Slot::Aniso2xWrap,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X2);
	createSampler(
	    Slot::Aniso2xClamp,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X2);
	createSampler(
	    Slot::Aniso2xMirror,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X2);
	createSampler(
	    Slot::Aniso4xWrap,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X4);
	createSampler(
	    Slot::Aniso4xClamp,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X4);
	createSampler(
	    Slot::Aniso4xMirror,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X4);
	createSampler(
	    Slot::Aniso8xWrap,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X8);
	createSampler(
	    Slot::Aniso8xClamp,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X8);
	createSampler(
	    Slot::Aniso8xMirror,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X8);
	createSampler(
	    Slot::Aniso16xWrap,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Wrap,
	    RhiSamplerAnisotropy::X16);
	createSampler(
	    Slot::Aniso16xClamp,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Clamp,
	    RhiSamplerAnisotropy::X16);
	createSampler(
	    Slot::Aniso16xMirror,
	    RhiSamplerMinMagFilter::Linear,
	    RhiSamplerMipFilter::Linear,
	    RhiSamplerAddressMode::Mirror,
	    RhiSamplerAnisotropy::X16);

	m_bInitialized = true;
}

D3D12SamplerLibrary::~D3D12SamplerLibrary() noexcept
{
	if (m_tableHandle && m_descriptorService)
	{
		m_descriptorService->ReleaseDescriptorTable(m_tableHandle);
		m_tableHandle = {};
	}
	m_bInitialized = false;
}

void D3D12SamplerLibrary::CreateSampler(Slot slot, const RhiSamplerDesc& samplerDesc)
{
	const std::uint32_t maxAnisotropy = static_cast<std::uint32_t>(samplerDesc.MaxAnisotropy);
	const bool isAnisotropic = maxAnisotropy > 1;

	D3D12_SAMPLER_DESC desc = {};
	desc.Filter = ToD3D12Filter(samplerDesc.MinMagFilter, samplerDesc.MipFilter, isAnisotropic);
	desc.AddressU = ToD3D12Address(samplerDesc.Address.U);
	desc.AddressV = desc.AddressU;
	desc.AddressW = desc.AddressU;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = maxAnisotropy;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	desc.MinLOD = 0.0f;
	desc.MaxLOD = samplerDesc.MipFilter == RhiSamplerMipFilter::None ? 0.0f : D3D12_FLOAT32_MAX;

	const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
	    D3D12TypeConversions::ToCpuDescriptor(m_descriptorService->GetDescriptorTableCpuHandle(m_tableHandle, static_cast<uint32_t>(slot)));

	m_rhi->GetDevice()->CreateSampler(&desc, cpuHandle);
}

RhiSamplerDesc D3D12SamplerLibrary::MakeSamplerDesc(
    RhiSamplerMinMagFilter minMagFilter,
    RhiSamplerMipFilter mipFilter,
    RhiSamplerAddressMode addressMode,
    RhiSamplerAnisotropy maxAnisotropy) noexcept
{
	return RhiSamplerDesc{
	    .MinMagFilter = minMagFilter,
	    .MipFilter = mipFilter,
	    .Address = MakeRhiSamplerAddressModes(addressMode),
	    .MaxAnisotropy = maxAnisotropy};
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

D3D12_FILTER D3D12SamplerLibrary::ToD3D12Filter(RhiSamplerMinMagFilter minMag, RhiSamplerMipFilter mip, bool anisotropic)
{
	if (anisotropic)
	{
		return D3D12_FILTER_ANISOTROPIC;
	}

	const uint32_t minMagBit = (minMag == RhiSamplerMinMagFilter::Linear) ? 1u : 0u;
	const uint32_t mipBit = (mip == RhiSamplerMipFilter::Linear) ? 1u : 0u;

	const uint32_t filterValue = (minMagBit << 4) | (minMagBit << 2) | mipBit;

	return static_cast<D3D12_FILTER>(filterValue);
}

D3D12_TEXTURE_ADDRESS_MODE D3D12SamplerLibrary::ToD3D12Address(RhiSamplerAddressMode address)
{
	switch (address)
	{
		case RhiSamplerAddressMode::Wrap:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case RhiSamplerAddressMode::Clamp:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case RhiSamplerAddressMode::Mirror:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	}
	Diagnostics::Fatal(g_samplerLibraryLogger, __FILE__, __LINE__, "D3D12 received an unsupported sampler address mode.");
}
