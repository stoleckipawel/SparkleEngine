#pragma once

#include "Capture/RhiCaptureService.h"

class D3D12RenderHardwareInterface;

class D3D12CaptureService final : public RhiCaptureService
{
  public:
	explicit D3D12CaptureService(D3D12RenderHardwareInterface& owner) noexcept;

	RhiCaptureResult CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept override;

  private:
	bool CaptureNativeTextureToBmp(
	    NativeResourceHandle resource,
	    ResourceState sourceState,
	    const std::filesystem::path& outputPath) noexcept;

	D3D12RenderHardwareInterface* m_owner = nullptr;
};
