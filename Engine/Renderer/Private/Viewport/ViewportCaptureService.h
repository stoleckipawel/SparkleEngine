#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Capture/RhiCaptureService.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class FrameGraph;
class RenderDeviceServices;

class ViewportCaptureService final
{
public:
	explicit ViewportCaptureService(RenderDeviceServices& deviceServices) noexcept;

	bool BeginCapture(
	    ViewportCaptureId id,
	    const ViewportCaptureRequest& request,
	    const ViewportRenderProducts& products,
	    FrameGraph* frameGraph,
	    std::uint64_t frameId,
	    std::uint64_t sceneGeneration,
	    std::uint64_t providerGeneration) noexcept;
	void Poll() noexcept;
	std::vector<ViewportCaptureReadback> TakeCompletedCaptures();

private:
	static constexpr std::size_t CaptureCapacity = 3;

	struct PendingCapture final
	{
		ViewportCaptureId Id;
		RhiCaptureTicket Ticket;
		std::uint64_t SceneGeneration = 0;
		std::uint64_t ProviderGeneration = 0;
	};

	void PublishCompleted(ViewportCaptureReadback readback);

	RenderDeviceServices& m_deviceServices;
	std::vector<std::unique_ptr<PendingCapture>> m_pendingCaptures;
	std::vector<ViewportCaptureReadback> m_completedCaptures;
};
