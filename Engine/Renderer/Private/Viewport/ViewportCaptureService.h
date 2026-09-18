#pragma once

#include "RHI/Public/Capture/RhiCaptureService.h"
#include "Viewport/ViewportCaptureCompletion.h"

#include <cstddef>
#include <cstdint>
#include <string>
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
	    FrameGraph& frameGraph,
	    std::uint64_t frameId,
	    std::uint64_t sceneGeneration,
	    std::uint64_t providerGeneration) noexcept;
	void Poll() noexcept;
	std::vector<ViewportCaptureCompletion> TakeCompletedCaptures();

private:
	struct CaptureSource;
	static bool ResolveSource(
	    const ViewportRenderProducts& products,
	    FrameGraph& frameGraph,
	    RenderOutputFlags output,
	    CaptureSource& source,
	    std::string& failureReason);

	struct PendingCapture final
	{
		ViewportCaptureId Id;
		RhiCaptureTicket Ticket;
		ViewportCaptureResult Result;
	};

	RenderDeviceServices& m_deviceServices;
	std::vector<PendingCapture> m_pendingCaptures;
	std::vector<ViewportCaptureCompletion> m_completedCaptures;
};
