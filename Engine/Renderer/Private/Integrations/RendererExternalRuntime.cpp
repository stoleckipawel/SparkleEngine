#include "PCH.h"
#include "Integrations/RendererExternalRuntime.h"

#include "RHI/Public/Core/RhiBackendSelection.h"
#include "Streamline/StreamlineRuntimeSupport.h"

RendererExternalRuntime::RendererExternalRuntime() noexcept
{
	m_backendConfiguration.BackendApi = ResolveDefaultRhiBackendApi();
	(void) InitializeSharedStreamlineRuntime(m_backendConfiguration.BackendApi);
	m_backendConfiguration.ExternalFeatureHooks = GetSharedStreamlineRhiHooks();
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
