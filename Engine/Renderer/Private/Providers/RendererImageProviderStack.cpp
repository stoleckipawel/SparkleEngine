#include "../PCH.h"
#include "Providers/RendererImageProviderStack.h"

#include "Providers/ImageProviderFrameContext.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionProviderFactory.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerProviderFactory.h"
#include "Upscaling/UpscalerSettings.h"

static const auto g_rendererImageProviderStackLogger = Logging::GetOrCreateLogger("Renderer.ImageProviders");

RendererImageProviderStack::RendererImageProviderStack(RenderHardwareInterface& renderHardwareInterface)
{
	Initialize(renderHardwareInterface);
}

RendererImageProviderStack::~RendererImageProviderStack() noexcept
{
	Shutdown();
}

void RendererImageProviderStack::Initialize(RenderHardwareInterface& renderHardwareInterface)
{
	const RhiCapabilities& capabilities = renderHardwareInterface.GetCapabilities();
	RhiInteropService& interop = renderHardwareInterface.GetInteropService();

	m_upscaler = CreateConfiguredUpscalerProvider();
	if (m_upscaler != nullptr &&
	    !m_upscaler->Initialize(
	        capabilities,
	        interop.GetDeviceQueueInterop(
	            RhiNativeInteropRequest{
	                .Consumer = ERhiNativeInteropConsumer::ExternalProvider,
	                .Reason = "Renderer upscaler provider initialization"})))
	{
		Diagnostics::Fatal(
		    g_rendererImageProviderStackLogger,
		    __FILE__,
		    __LINE__,
		    "The configured renderer upscaler could not initialize on the selected RHI backend and adapter.");
	}

	m_rayReconstruction = CreateConfiguredRayReconstructionProvider();
	if (m_rayReconstruction != nullptr &&
	    !m_rayReconstruction->Initialize(
	        capabilities,
	        interop.GetDeviceQueueInterop(
	            RhiNativeInteropRequest{
	                .Consumer = ERhiNativeInteropConsumer::ExternalProvider,
	                .Reason = "Renderer ray-reconstruction provider initialization"})))
	{
		Diagnostics::Fatal(
		    g_rendererImageProviderStackLogger,
		    __FILE__,
		    __LINE__,
		    "The configured ray-reconstruction provider could not initialize on the selected RHI backend and adapter.");
	}

	m_resetHistoryPending = true;
}

void RendererImageProviderStack::Shutdown() noexcept
{
	if (m_upscaler != nullptr)
	{
		m_upscaler->Shutdown();
		m_upscaler.reset();
	}
	if (m_rayReconstruction != nullptr)
	{
		m_rayReconstruction->Shutdown();
		m_rayReconstruction.reset();
	}
}

void RendererImageProviderStack::ResetHistory() noexcept
{
	m_resetHistoryPending = true;
}

void RendererImageProviderStack::SetupFrame(const ImageProviderFrameContext& frameContext)
{
	ImageProviderFrameContext providerFrame = frameContext;
	providerFrame.ResetHistory |= m_resetHistoryPending;
	m_resetHistoryPending = false;
	if (m_upscaler != nullptr)
	{
		m_upscaler->SetupFrame(providerFrame);
	}
	if (m_rayReconstruction != nullptr)
	{
		m_rayReconstruction->SetupFrame(providerFrame);
	}
}

RenderViewportExtent RendererImageProviderStack::ResolveRenderExtent(
    RenderViewportExtent outputExtent,
    ImageProviderPipeline pipeline) noexcept
{
	if (pipeline == ImageProviderPipeline::RayReconstruction && m_rayReconstruction != nullptr)
	{
		return m_rayReconstruction->ResolveRenderExtent(outputExtent);
	}
	if (m_upscaler != nullptr)
	{
		return m_upscaler->ResolveRenderExtent(outputExtent);
	}
	return outputExtent;
}

ImageProviderGraphKey RendererImageProviderStack::GetFrameGraphKey() const noexcept
{
	return ImageProviderGraphKey{
	    .UpscalerProvider = GetUpscalerProviderSelectionKey(),
	    .RayReconstructionMode = GetRayReconstructionModeKey()};
}

ImageProviderPassContext RendererImageProviderStack::BuildPassContext() noexcept
{
	return ImageProviderPassContext{
	    .Upscaling = m_upscaler.get(),
	    .RayReconstruction = m_rayReconstruction.get()};
}
