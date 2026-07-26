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

class RendererImageProviderStackOperations final
{
  public:
	template <typename TProvider>
	static bool InitializeImageProvider(
	    TProvider& provider,
	    const RhiCapabilities& capabilities,
	    RhiInteropService& interop,
	    ERhiNativeInteropConsumer consumer,
	    const char* reason)
	{
		return provider.Initialize(
		    capabilities,
		    interop.GetDeviceQueueInterop(RhiNativeInteropRequest{.Consumer = consumer, .Reason = reason}));
	}
};

RendererImageProviderStack::RendererImageProviderStack(RenderHardwareInterface& renderHardware)
{
	Initialize(renderHardware);
}

RendererImageProviderStack::~RendererImageProviderStack() noexcept
{
	Shutdown();
}

void RendererImageProviderStack::Initialize(RenderHardwareInterface& renderHardware)
{
	const RhiCapabilities& capabilities = renderHardware.GetCapabilities();
	RhiInteropService& interop = renderHardware.GetInteropService();

	m_upscaler = CreateConfiguredUpscalerProvider();
	if (m_upscaler != nullptr && !RendererImageProviderStackOperations::InitializeImageProvider(
	                               *m_upscaler,
	                               capabilities,
	                               interop,
	                               ERhiNativeInteropConsumer::UpscalerProvider,
	                               "Renderer upscaler provider initialization"))
	{
		m_upscaler->Shutdown();
		m_upscaler.reset();
	}

	m_rayReconstruction = CreateConfiguredRayReconstructionProvider();
	m_rayReconstructionRequested = m_rayReconstruction != nullptr;
	if (m_rayReconstruction != nullptr && !RendererImageProviderStackOperations::InitializeImageProvider(
	                                         *m_rayReconstruction,
	                                         capabilities,
	                                         interop,
	                                         ERhiNativeInteropConsumer::RayReconstructionProvider,
	                                         "Renderer ray-reconstruction provider initialization"))
	{
		m_rayReconstruction->Shutdown();
		m_rayReconstruction.reset();
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
	m_rayReconstructionRequested = false;
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
	if (pipeline == ImageProviderPipeline::RayReconstruction && m_rayReconstructionRequested)
	{
		return m_rayReconstruction != nullptr ? m_rayReconstruction->ResolveRenderExtent(outputExtent) : outputExtent;
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

RendererImageProviderPassServices RendererImageProviderStack::BuildPassServices() noexcept
{
	return RendererImageProviderPassServices{
	    .Upscaling = m_upscaler.get(),
	    .RayReconstruction = m_rayReconstruction.get()};
}
