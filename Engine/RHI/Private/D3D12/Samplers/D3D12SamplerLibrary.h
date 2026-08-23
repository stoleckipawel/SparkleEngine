#pragma once

#include "Descriptors/RhiDescriptorService.h"
#include <d3d12.h>

#include <cstdint>

class D3D12Rhi;

class D3D12SamplerLibrary
{
  public:
	enum class Slot : uint32_t
	{
		PointMipPointWrap = 0,
		PointMipPointClamp,
		PointMipPointMirror,
		PointMipLinearWrap,
		PointMipLinearClamp,
		PointMipLinearMirror,
		PointNoMipWrap,
		PointNoMipClamp,
		PointNoMipMirror,

		LinearMipPointWrap,
		LinearMipPointClamp,
		LinearMipPointMirror,
		LinearMipLinearWrap,
		LinearMipLinearClamp,
		LinearMipLinearMirror,
		LinearNoMipWrap,
		LinearNoMipClamp,
		LinearNoMipMirror,

		Aniso1xWrap,
		Aniso1xClamp,
		Aniso1xMirror,
		Aniso2xWrap,
		Aniso2xClamp,
		Aniso2xMirror,
		Aniso4xWrap,
		Aniso4xClamp,
		Aniso4xMirror,
		Aniso8xWrap,
		Aniso8xClamp,
		Aniso8xMirror,
		Aniso16xWrap,
		Aniso16xClamp,
		Aniso16xMirror,

		Count
	};

	D3D12SamplerLibrary(D3D12Rhi& rhi, RhiDescriptorService& descriptorService);
	~D3D12SamplerLibrary() noexcept;

	D3D12SamplerLibrary(const D3D12SamplerLibrary&) = delete;
	D3D12SamplerLibrary& operator=(const D3D12SamplerLibrary&) = delete;
	D3D12SamplerLibrary(D3D12SamplerLibrary&&) = delete;
	D3D12SamplerLibrary& operator=(D3D12SamplerLibrary&&) = delete;

	bool IsInitialized() const noexcept { return m_bInitialized; }
	RhiDescriptorTableHandle GetTableHandle() const noexcept { return m_tableHandle; }
	static constexpr uint32_t GetSamplerCount() noexcept { return static_cast<uint32_t>(Slot::Count); }
	static bool TryGetSlot(const RhiSamplerDesc& samplerDesc, Slot& outSlot) noexcept;

  private:
	void CreateSampler(Slot slot, const RhiSamplerDesc& desc);
	static RhiSamplerDesc MakeSamplerDesc(
	    RhiSamplerMinMagFilter minMagFilter,
	    RhiSamplerMipFilter mipFilter,
	    RhiSamplerAddressMode addressMode,
	    RhiSamplerAnisotropy maxAnisotropy) noexcept;
	static bool TryGetAddressOffset(RhiSamplerAddressMode addressMode, std::uint32_t& outOffset) noexcept;
	static bool TryGetUniformAddressMode(const RhiSamplerDesc& samplerDesc, RhiSamplerAddressMode& outAddressMode) noexcept;
	static bool TryGetPointSlot(RhiSamplerMipFilter mipFilter, RhiSamplerAddressMode addressMode, Slot& outSlot) noexcept;
	static bool TryGetLinearSlot(RhiSamplerMipFilter mipFilter, RhiSamplerAddressMode addressMode, Slot& outSlot) noexcept;
	static bool TryGetAnisotropicSlot(RhiSamplerAnisotropy maxAnisotropy, RhiSamplerAddressMode addressMode, Slot& outSlot) noexcept;
	static D3D12_FILTER ToD3D12Filter(RhiSamplerMinMagFilter minMag, RhiSamplerMipFilter mip, bool anisotropic);
	static D3D12_TEXTURE_ADDRESS_MODE ToD3D12Address(RhiSamplerAddressMode address);

	bool m_bInitialized = false;
	D3D12Rhi* m_rhi = nullptr;
	RhiDescriptorService* m_descriptorService = nullptr;
	RhiDescriptorTableHandle m_tableHandle = {};
	uint32_t m_descriptorSize = 0;
};
