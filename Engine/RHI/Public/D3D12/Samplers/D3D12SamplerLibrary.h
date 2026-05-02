#pragma once

#include "Interop/RenderHardwareInterface.h"
#include <d3d12.h>

#include <cstdint>

class D3D12Rhi;

class D3D12SamplerLibrary
{
  public:
	enum class MinMagFilter : uint8_t
	{
		Point,
		Linear
	};

	enum class MipFilter : uint8_t
	{
		None,
		Point,
		Linear
	};

	enum class AddressMode : uint8_t
	{
		Wrap,
		Clamp,
		Mirror
	};

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

	D3D12SamplerLibrary(D3D12Rhi& rhi, RenderHardwareInterface& renderHardwareInterface);
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
	struct SamplerConfig
	{
		MinMagFilter minMag;
		MipFilter mip;
		AddressMode address;
		uint32_t maxAnisotropy;
	};

	void CreateSampler(Slot slot, const SamplerConfig& config);
	static bool TryGetAddressOffset(RhiSamplerAddressMode addressMode, std::uint32_t& outOffset) noexcept;
	static bool TryGetUniformAddressMode(const RhiSamplerDesc& samplerDesc, RhiSamplerAddressMode& outAddressMode) noexcept;
	static bool TryGetPointSlot(RhiSamplerMipFilter mipFilter, RhiSamplerAddressMode addressMode, Slot& outSlot) noexcept;
	static bool TryGetLinearSlot(RhiSamplerMipFilter mipFilter, RhiSamplerAddressMode addressMode, Slot& outSlot) noexcept;
	static bool TryGetAnisotropicSlot(RhiSamplerAnisotropy maxAnisotropy, RhiSamplerAddressMode addressMode, Slot& outSlot) noexcept;
	static D3D12_FILTER ToD3D12Filter(MinMagFilter minMag, MipFilter mip, bool anisotropic);
	static D3D12_TEXTURE_ADDRESS_MODE ToD3D12Address(AddressMode address);

	bool m_bInitialized = false;
	D3D12Rhi* m_rhi = nullptr;
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiDescriptorTableHandle m_tableHandle = {};
	uint32_t m_descriptorSize = 0;
};
