#pragma once

#include "Capture/RhiCaptureService.h"

class D3D12Rhi;

class D3D12CaptureService final : public RhiCaptureService
{
  public:
	explicit D3D12CaptureService(D3D12Rhi& rhi) noexcept;

	RhiCaptureResult CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept override;

  private:
	bool CaptureNativeTextureToBmp(
	    RhiResourceHandle resource,
	    ResourceState sourceState,
	    const std::filesystem::path& outputPath) noexcept;

	D3D12Rhi* m_rhi = nullptr;
};
