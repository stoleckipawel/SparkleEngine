#pragma once

#include "Capture/RhiCaptureService.h"

#include <memory>
#include <vector>

class D3D12Rhi;

class D3D12CaptureService final : public RhiCaptureService
{
  public:
	explicit D3D12CaptureService(D3D12Rhi& rhi) noexcept;
	~D3D12CaptureService() noexcept override;

	RhiCaptureTicket BeginTextureReadback(
	    const RhiTextureCaptureRequest& request) noexcept override;
	bool TryTakeTextureReadback(
	    RhiCaptureTicket ticket,
	    RhiCaptureReadback& readback) noexcept override;
	void CancelTextureReadback(RhiCaptureTicket ticket) noexcept override;

  private:
	struct PendingReadback;
	PendingReadback* FindPending(RhiCaptureTicket ticket) noexcept;
	void DrainCancelledReadbacks() noexcept;

	D3D12Rhi* m_rhi = nullptr;
	std::vector<std::unique_ptr<PendingReadback>> m_pendingReadbacks;
	std::uint64_t m_nextTicket = 1;
};
