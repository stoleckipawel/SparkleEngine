#include "PCH.h"
#include "Integrations/RendererExternalRuntime.h"

#include "RHI/Public/Core/RhiBackendSelection.h"
#include "Streamline/StreamlineRuntimeSupport.h"

RendererExternalRuntime::RendererExternalRuntime() noexcept
{
	m_backendConfiguration.BackendApi = ResolveDefaultRhiBackendApi();
	(void) InitializeSharedStreamlineRuntime(m_backendConfiguration.BackendApi);
	m_backendConfiguration.D3D12InterposerHooks = GetSharedStreamlineD3D12InterposerHooks();
}

RendererExternalRuntime::~RendererExternalRuntime() noexcept
{
	m_owner.AssertAccess();
	ShutdownSharedStreamlineRuntime();
}

const RendererBackendConfiguration& RendererExternalRuntime::GetBackendConfiguration() const noexcept
{
	m_owner.AssertAccess();
	return m_backendConfiguration;
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
