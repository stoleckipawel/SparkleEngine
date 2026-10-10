#include "PCH.h"
#include "Integrations/RendererExternalRuntime.h"

#include "Streamline/StreamlineRuntimeSupport.h"

RendererExternalRuntime::RendererExternalRuntime(RendererGraphicsLaunch graphicsLaunch) noexcept
{
	m_deviceLaunch.BackendApi = graphicsLaunch.BackendApi;
#if SPARKLE_WITH_EXTERNAL_CAPTURE
	if (graphicsLaunch.CaptureProvider != ExternalCaptureProvider::None)
	{
		m_externalCapture = std::make_unique<RhiExternalCapture>(graphicsLaunch.BackendApi, graphicsLaunch.CaptureProvider);
		m_deviceLaunch.ExternalCapture = m_externalCapture.get();
	}
#endif
	m_deviceLaunch.InterposerHooks = InitializeSharedStreamlineRuntime(m_deviceLaunch.BackendApi);
}

RendererExternalRuntime::~RendererExternalRuntime() noexcept
{
	m_owner.AssertAccess();
	ShutdownSharedStreamlineRuntime();
}

const RhiDeviceLaunch& RendererExternalRuntime::GetDeviceLaunch() const noexcept
{
	m_owner.AssertAccess();
	return m_deviceLaunch;
}

void RendererExternalRuntime::BeginSimulationFrame(std::uint64_t frameId) noexcept
{
	m_owner.AssertAccess();
	SetSharedStreamlineFrameMarker(ERhiFrameLatencyMarker::SimulationStart, frameId);
}

void RendererExternalRuntime::EndSimulationFrame(std::uint64_t frameId) noexcept
{
	m_owner.AssertAccess();
	SetSharedStreamlineFrameMarker(ERhiFrameLatencyMarker::SimulationEnd, frameId);
}
