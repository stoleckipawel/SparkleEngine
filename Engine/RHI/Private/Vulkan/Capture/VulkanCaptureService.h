#pragma once

#include "Capture/RhiCaptureService.h"
#include "Vulkan/VulkanIncludes.h"

class VulkanRenderHardwareInterface;

class VulkanCaptureService final : public RhiCaptureService
{
  public:
	explicit VulkanCaptureService(VulkanRenderHardwareInterface& owner) noexcept;

	RhiCaptureResult CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept override;

  private:
	bool CaptureNativeTextureToBmp(
	    NativeResourceHandle resource,
	    std::uint32_t width,
	    std::uint32_t height,
	    const std::filesystem::path& outputPath) noexcept;

	VulkanRenderHardwareInterface* m_owner = nullptr;
};
