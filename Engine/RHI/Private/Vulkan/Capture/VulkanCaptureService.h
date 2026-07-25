#pragma once

#include "Capture/RhiCaptureService.h"
#include "Vulkan/VulkanIncludes.h"

#include <memory>
#include <vector>

class VulkanRhi;

class VulkanCaptureService final : public RhiCaptureService
{
  public:
	explicit VulkanCaptureService(VulkanRhi& rhi) noexcept;
	~VulkanCaptureService() noexcept override;

	RhiCaptureTicket BeginTextureReadback(
	    const RhiTextureCaptureRequest& request) noexcept override;
	bool TryTakeTextureReadback(
	    RhiCaptureTicket ticket,
	    RhiCaptureReadback& readback) noexcept override;
	void CancelTextureReadback(RhiCaptureTicket ticket) noexcept override;

  private:
	struct PendingReadback;
	PendingReadback* FindPending(RhiCaptureTicket ticket) noexcept;
	void ReleasePending(std::size_t index) noexcept;

	VulkanRhi* m_rhi = nullptr;
	std::vector<std::unique_ptr<PendingReadback>> m_pendingReadbacks;
	std::uint64_t m_nextTicket = 1;
};
