#include "../PCH.h"
#include "Providers/RendererImageProviderStack.h"

#include "Providers/ImageProviderFrameInput.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionProviderFactory.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerProviderFactory.h"
#include "Upscaling/UpscalerSettings.h"

#include <algorithm>
#include <array>
#include <limits>

static const auto g_rendererImageProviderStackLogger = Logging::GetOrCreateLogger("Renderer.ImageProviders");

RendererImageProviderStack::RendererImageProviderStack(
    RenderHardwareInterface& renderHardwareInterface,
    RenderDeviceServices& deviceServices) :
    m_renderHardwareInterface(renderHardwareInterface),
    m_deviceServices(deviceServices)
{
	Initialize();
}

RendererImageProviderStack::~RendererImageProviderStack() noexcept
{
	Shutdown();
}

void RendererImageProviderStack::Initialize()
{
	const RhiCapabilities& capabilities = m_renderHardwareInterface.GetCapabilities();
	RhiInteropService& interop = m_renderHardwareInterface.GetInteropService();

	m_upscaler = CreateConfiguredUpscalerProvider();
	if (m_upscaler != nullptr
	    && !m_upscaler->Initialize(
	        capabilities,
	        interop.GetDeviceQueueInterop(
	            RhiNativeInteropRequest{
	                .Consumer = ERhiNativeInteropConsumer::ExternalProvider,
	                .Reason = "Renderer upscaler provider initialization"})))
	{
		m_upscaler->Shutdown();
		m_upscaler.reset();
		CVarUpscalerProvider.Set(EUpscalerProviderKind::Linear);
		g_rendererImageProviderStackLogger->warn(
		    "The configured renderer upscaler could not initialize on the selected RHI backend and adapter; falling back to linear "
		    "upscaling.");
	}

	m_rayReconstruction = CreateConfiguredRayReconstructionProvider();
	if (m_rayReconstruction != nullptr
	    && !m_rayReconstruction->Initialize(
	        capabilities,
	        interop.GetDeviceQueueInterop(
	            RhiNativeInteropRequest{
	                .Consumer = ERhiNativeInteropConsumer::ExternalProvider,
	                .Reason = "Renderer ray-reconstruction provider initialization"})))
	{
		m_rayReconstruction->Shutdown();
		m_rayReconstruction.reset();
		CVarRayReconstructionMode.Set(EngineRayReconstructionMode::Off);
		g_rendererImageProviderStackLogger->warn(
		    "The configured ray-reconstruction provider could not initialize on the selected RHI backend and adapter; disabling ray "
		    "reconstruction.");
	}

	m_resetHistoryPending = true;
}

void RendererImageProviderStack::Shutdown() noexcept
{
	ShutdownProviders(m_upscaler, m_rayReconstruction);
	for (RetiredGeneration& generation : m_retiredGenerations)
	{
		ShutdownProviders(generation.Upscaler, generation.RayReconstruction);
	}
	m_retiredGenerations.clear();
}

void RendererImageProviderStack::ShutdownProviders(
    std::unique_ptr<IUpscalerProvider>& upscaler,
    std::unique_ptr<IRayReconstructionProvider>& rayReconstruction) noexcept
{
	if (upscaler != nullptr)
	{
		upscaler->Shutdown();
		upscaler.reset();
	}
	if (rayReconstruction != nullptr)
	{
		rayReconstruction->Shutdown();
		rayReconstruction.reset();
	}
}

void RendererImageProviderStack::Refresh() noexcept
{
	if (m_generation == (std::numeric_limits<std::uint64_t>::max)())
	{
		Diagnostics::Fatal(g_rendererImageProviderStackLogger, __FILE__, __LINE__, "Renderer image-provider generation exhausted.");
	}

	RhiSubmissionState lastUse;
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		lastUse.MarkUsed(m_deviceServices.GetLastSubmittedToken(static_cast<ERhiQueueType>(queueIndex)));
	}
	m_retiredGenerations.push_back(
	    RetiredGeneration{.LastUse = lastUse, .Upscaler = std::move(m_upscaler), .RayReconstruction = std::move(m_rayReconstruction)});
	Initialize();
	++m_generation;
}

void RendererImageProviderStack::PollRetiredGenerations() noexcept
{
	m_retiredGenerations.erase(
	    std::remove_if(
	        m_retiredGenerations.begin(),
	        m_retiredGenerations.end(),
	        [this](RetiredGeneration& generation) noexcept
	        {
		        std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
		        const std::size_t tokenCount = generation.LastUse.CopyTokens(tokens);
		        for (std::size_t tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex)
		        {
			        if (!m_deviceServices.IsSubmissionComplete(tokens[tokenIndex]))
			        {
				        return false;
			        }
		        }
		        ShutdownProviders(generation.Upscaler, generation.RayReconstruction);
		        return true;
	        }),
	    m_retiredGenerations.end());
}

void RendererImageProviderStack::ResetHistory() noexcept
{
	m_resetHistoryPending = true;
}

void RendererImageProviderStack::SetupFrame(const ImageProviderFrameInput& frameInput)
{
	ImageProviderFrameInput providerInput = frameInput;
	providerInput.ResetHistory |= m_resetHistoryPending;
	m_resetHistoryPending = false;
	if (m_upscaler != nullptr)
	{
		m_upscaler->SetupFrame(providerInput);
	}
	if (m_rayReconstruction != nullptr)
	{
		m_rayReconstruction->SetupFrame(providerInput);
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
