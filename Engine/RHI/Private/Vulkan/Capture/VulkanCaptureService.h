#pragma once

#include "Capture/RhiCaptureService.h"
#include "Vulkan/VulkanIncludes.h"

class VulkanRhi;

class VulkanCaptureService final : public RhiCaptureService
{
  public:
	explicit VulkanCaptureService(VulkanRhi& rhi) noexcept;

	RhiCaptureResult CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept override;

  private:
	bool CaptureNativeTextureToBmp(
	    RhiResourceHandle resource,
	    std::uint32_t width,
	    std::uint32_t height,
	    PixelFormat sourceFormat,
	    ResourceState sourceState,
	    const std::filesystem::path& outputPath) noexcept;

	VulkanRhi* m_rhi = nullptr;
};
