#pragma once

#include "Capture/RhiCaptureService.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class VulkanRhi;

class VulkanCaptureService final : public RhiCaptureService
{
public:
	explicit VulkanCaptureService(VulkanRhi& rhi) noexcept;
	~VulkanCaptureService() noexcept override;

	RhiCaptureTicket BeginTextureReadback(const RhiTextureCaptureRequest& request) noexcept override;
	bool TryTakeTextureReadback(RhiCaptureTicket ticket, RhiCaptureReadback& readback) noexcept override;
	void CancelTextureReadback(RhiCaptureTicket ticket) noexcept override;

private:
	struct PendingReadback;
	void DrainCancelledReadbacks() noexcept;
	void ReleasePending(std::size_t index) noexcept;

	VulkanRhi& m_rhi;
	std::vector<std::unique_ptr<PendingReadback>> m_pendingReadbacks;
	std::uint64_t m_nextTicket = 1;
};
