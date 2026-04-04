#pragma once

#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include <d3d12.h>

class D3D12DescriptorHeapManager;
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
		Point,
		Linear,
		None
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

	D3D12SamplerLibrary(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager);
	~D3D12SamplerLibrary() noexcept;

	D3D12SamplerLibrary(const D3D12SamplerLibrary&) = delete;
	D3D12SamplerLibrary& operator=(const D3D12SamplerLibrary&) = delete;
	D3D12SamplerLibrary(D3D12SamplerLibrary&&) = delete;
	D3D12SamplerLibrary& operator=(D3D12SamplerLibrary&&) = delete;

	bool IsInitialized() const noexcept { return m_bInitialized; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetTableGPUHandle() const noexcept { return m_tableHandle.GetGPU(); }
	static constexpr uint32_t GetSamplerCount() noexcept { return static_cast<uint32_t>(Slot::Count); }

  private:
	struct SamplerConfig
	{
		MinMagFilter minMag;
		MipFilter mip;
		AddressMode address;
		uint32_t maxAnisotropy;
	};

	void CreateSampler(Slot slot, const SamplerConfig& config);
	static D3D12_FILTER ToD3D12Filter(MinMagFilter minMag, MipFilter mip, bool anisotropic);
	static D3D12_TEXTURE_ADDRESS_MODE ToD3D12Address(AddressMode address);

	bool m_bInitialized = false;
	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHandle m_tableHandle;
	uint32_t m_descriptorSize = 0;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
};
