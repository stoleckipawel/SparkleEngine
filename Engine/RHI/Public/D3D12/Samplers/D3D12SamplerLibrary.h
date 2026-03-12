// ============================================================================
// D3D12SamplerLibrary.h
// Pre-built sampler descriptor table for shader binding.
// ----------------------------------------------------------------------------
#pragma once

#include "D3D12DescriptorHandle.h"
#include "D3D12RootBindings.h"
#include <d3d12.h>

class D3D12DescriptorHeapManager;
class D3D12Rhi;

class D3D12SamplerLibrary
{
  public:
	// -------------------------------------------------------------------------
	// Filter Configuration
	// -------------------------------------------------------------------------
	enum class MinMagFilter : uint8_t
	{
		Point,
		Linear
	};

	enum class MipFilter : uint8_t
	{
		Point,
		Linear,
		None  // Disables mipmapping (MaxLOD = 0)
	};

	enum class AddressMode : uint8_t
	{
		Wrap,
		Clamp,
		Mirror
	};

	// -------------------------------------------------------------------------
	// Sampler Slots
	// -------------------------------------------------------------------------
	enum class Slot : uint32_t
	{
		// Point MinMag
		PointMipPointWrap = RootBindings::SamplerRegister::PointMipPointWrap,
		PointMipPointClamp = RootBindings::SamplerRegister::PointMipPointClamp,
		PointMipPointMirror = RootBindings::SamplerRegister::PointMipPointMirror,
		PointMipLinearWrap = RootBindings::SamplerRegister::PointMipLinearWrap,
		PointMipLinearClamp = RootBindings::SamplerRegister::PointMipLinearClamp,
		PointMipLinearMirror = RootBindings::SamplerRegister::PointMipLinearMirror,
		PointNoMipWrap = RootBindings::SamplerRegister::PointNoMipWrap,
		PointNoMipClamp = RootBindings::SamplerRegister::PointNoMipClamp,
		PointNoMipMirror = RootBindings::SamplerRegister::PointNoMipMirror,

		// Linear MinMag
		LinearMipPointWrap = RootBindings::SamplerRegister::LinearMipPointWrap,
		LinearMipPointClamp = RootBindings::SamplerRegister::LinearMipPointClamp,
		LinearMipPointMirror = RootBindings::SamplerRegister::LinearMipPointMirror,
		LinearMipLinearWrap = RootBindings::SamplerRegister::LinearMipLinearWrap,
		LinearMipLinearClamp = RootBindings::SamplerRegister::LinearMipLinearClamp,
		LinearMipLinearMirror = RootBindings::SamplerRegister::LinearMipLinearMirror,
		LinearNoMipWrap = RootBindings::SamplerRegister::LinearNoMipWrap,
		LinearNoMipClamp = RootBindings::SamplerRegister::LinearNoMipClamp,
		LinearNoMipMirror = RootBindings::SamplerRegister::LinearNoMipMirror,

		// Anisotropic
		Aniso1xWrap = RootBindings::SamplerRegister::Aniso1xWrap,
		Aniso1xClamp = RootBindings::SamplerRegister::Aniso1xClamp,
		Aniso1xMirror = RootBindings::SamplerRegister::Aniso1xMirror,
		Aniso2xWrap = RootBindings::SamplerRegister::Aniso2xWrap,
		Aniso2xClamp = RootBindings::SamplerRegister::Aniso2xClamp,
		Aniso2xMirror = RootBindings::SamplerRegister::Aniso2xMirror,
		Aniso4xWrap = RootBindings::SamplerRegister::Aniso4xWrap,
		Aniso4xClamp = RootBindings::SamplerRegister::Aniso4xClamp,
		Aniso4xMirror = RootBindings::SamplerRegister::Aniso4xMirror,
		Aniso8xWrap = RootBindings::SamplerRegister::Aniso8xWrap,
		Aniso8xClamp = RootBindings::SamplerRegister::Aniso8xClamp,
		Aniso8xMirror = RootBindings::SamplerRegister::Aniso8xMirror,
		Aniso16xWrap = RootBindings::SamplerRegister::Aniso16xWrap,
		Aniso16xClamp = RootBindings::SamplerRegister::Aniso16xClamp,
		Aniso16xMirror = RootBindings::SamplerRegister::Aniso16xMirror,

		Count = RootBindings::SamplerRegister::Count
	};

	// -------------------------------------------------------------------------
	// Lifecycle
	// -------------------------------------------------------------------------
	D3D12SamplerLibrary(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager);
	~D3D12SamplerLibrary() noexcept;

	D3D12SamplerLibrary(const D3D12SamplerLibrary&) = delete;
	D3D12SamplerLibrary& operator=(const D3D12SamplerLibrary&) = delete;
	D3D12SamplerLibrary(D3D12SamplerLibrary&&) = delete;
	D3D12SamplerLibrary& operator=(D3D12SamplerLibrary&&) = delete;

	// -------------------------------------------------------------------------
	// Accessors
	// -------------------------------------------------------------------------
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
