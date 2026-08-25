#include "PCH.h"
#include "Host/RendererHost.h"

#include "Diagnostics/RendererMemoryMonitor.h"
#include "Host/RendererBackendOwner.h"
#include "Host/RendererBackendConfiguration.h"
#include "Meshes/GpuMeshCache.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Providers/RendererImageProviderStack.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "Scene/RenderScene.h"
#include "Scene/Preparation/RenderScenePreparation.h"
#include "Textures/TextureCache.h"
#include "Window/Window.h"
#include "View/RenderViewBuilder.h"
#include "View/RenderViewPreparation.h"
#include "View/RenderViewState.h"

RendererHost::RendererHost(
    Window& window,
    const RendererBackendConfiguration& backendConfiguration,
    TaskExecutor& taskExecutor,
    TaskScope& applicationTaskScope) noexcept :
    m_window(&window),
    m_taskExecutor(&taskExecutor)
{
	InitializeCoreRuntime(backendConfiguration, taskExecutor, applicationTaskScope);
	InitializeSceneRuntime(taskExecutor, applicationTaskScope);
}

RendererHost::~RendererHost() noexcept = default;

RenderDeviceServices& RendererHost::GetDeviceServices() noexcept
{
	return m_backendOwner->GetDeviceServices();
}

const RenderDeviceServices& RendererHost::GetDeviceServices() const noexcept
{
	return m_backendOwner->GetDeviceServices();
}
void RendererHost::InitializeCoreRuntime(
    const RendererBackendConfiguration& backendConfiguration,
    TaskExecutor& taskExecutor,
    TaskScope& applicationTaskScope) noexcept
{
	m_backendOwner = std::make_unique<RendererBackendOwner>(*m_window, backendConfiguration);
	RenderHardwareInterface& renderHardwareInterface = GetDeviceServices().GetRenderHardwareInterface();
	{
		m_renderPassRuntimeCache = std::make_unique<RenderPassRuntimeCache>(GetDeviceServices());
	}
	{
		m_gpuMeshCache = std::make_unique<GpuMeshCache>(renderHardwareInterface, GetDeviceServices(), taskExecutor, applicationTaskScope);
	}

	RenderDiagnostics& backendDiagnostics = renderHardwareInterface.GetDiagnostics();
	m_imageProviders = std::make_unique<RendererImageProviderStack>(renderHardwareInterface, GetDeviceServices());

	m_memoryMonitor = std::make_unique<RendererMemoryMonitor>(backendDiagnostics);
}

void RendererHost::InitializeSceneRuntime(TaskExecutor& taskExecutor, TaskScope& applicationTaskScope) noexcept
{
	RenderHardwareInterface& renderHardwareInterface = GetDeviceServices().GetRenderHardwareInterface();
	const RayTracingCapabilityReport rayTracingCapabilities = BuildRayTracingCapabilityReport(renderHardwareInterface.GetCapabilities());
	m_textureCache = std::make_unique<TextureCache>(
	    renderHardwareInterface.GetResourceService(),
	    renderHardwareInterface.GetDescriptorService(),
	    renderHardwareInterface.GetUploadService(),
	    GetDeviceServices(),
	    taskExecutor,
	    applicationTaskScope);
	m_renderScenePreparation = std::make_unique<RenderScenePreparation>(taskExecutor, *m_gpuMeshCache, *m_textureCache);
	m_renderViewBuilder = std::make_unique<RenderViewBuilder>();
	m_renderViewPreparation = std::make_unique<RenderViewPreparation>(taskExecutor);
	m_renderViewState = std::make_unique<RenderViewState>();
	m_renderScene = std::make_unique<RenderScene>(
	    &GetDeviceServices(),
	    *m_gpuMeshCache,
	    *m_textureCache,
	    renderHardwareInterface,
	    rayTracingCapabilities);
}
