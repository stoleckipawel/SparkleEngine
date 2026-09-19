#include "PCH.h"
#include "Host/RendererHost.h"

#include "Diagnostics/RendererMemoryMonitor.h"
#include "Frame/FramePipeline.h"
#include "Host/RendererBackendOwner.h"
#include "Host/RendererBackendConfiguration.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"

RendererHost::RendererHost(Window& window, const RendererBackendConfiguration& backendConfiguration) noexcept :
    m_window(window)
{
	m_backendOwner = std::make_unique<RendererBackendOwner>(m_window, backendConfiguration);
	RenderDeviceServices& deviceServices = m_backendOwner->GetDeviceServices();
	RenderHardwareInterface& renderHardwareInterface = deviceServices.GetRenderHardwareInterface();
	m_renderPassRuntimeCache = std::make_unique<RenderPassRuntimeCache>(deviceServices);
	RenderDiagnostics& backendDiagnostics = renderHardwareInterface.GetDiagnostics();
	m_memoryMonitor = std::make_unique<RendererMemoryMonitor>(backendDiagnostics);
}

RendererHost::~RendererHost() noexcept = default;

void RendererHost::ReloadShaders()
{
	m_renderPassRuntimeCache->ReloadShaders();
}

std::uint64_t RendererHost::GetShaderGeneration() const noexcept
{
	return m_renderPassRuntimeCache->GetShaderGeneration();
}

RendererMemoryDiagnosticsSnapshot RendererHost::CaptureMemoryDiagnostics() const
{
	return m_memoryMonitor->GetLatestSnapshot();
}

void RendererHost::SettleForShutdown() noexcept
{
	m_backendOwner->GetDeviceServices().SettleForShutdown();
}

std::unique_ptr<FramePipeline> RendererHost::CreateFramePipeline(
    TaskExecutor& taskExecutor,
    TaskScope& assetTaskParentScope,
    bool enableUiRenderPackets) noexcept
{
	RenderDeviceServices& deviceServices = m_backendOwner->GetDeviceServices();
	return std::unique_ptr<FramePipeline>(new FramePipeline(
	    m_window,
	    deviceServices,
	    *m_renderPassRuntimeCache,
	    *m_memoryMonitor,
	    taskExecutor,
	    assetTaskParentScope,
	    enableUiRenderPackets));
}
