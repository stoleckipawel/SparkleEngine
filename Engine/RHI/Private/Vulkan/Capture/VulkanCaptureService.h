#pragma once

#include "Capture/RhiCaptureService.h"

class VulkanRenderHardwareInterface;

class VulkanCaptureService final : public RhiCaptureService
{
  public:
	explicit VulkanCaptureService(VulkanRenderHardwareInterface& owner) noexcept;

	RhiCaptureResult CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept override;

  private:
	VulkanRenderHardwareInterface* m_owner = nullptr;
};
