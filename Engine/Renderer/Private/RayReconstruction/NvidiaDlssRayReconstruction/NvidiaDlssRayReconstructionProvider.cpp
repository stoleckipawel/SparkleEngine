#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlssRayReconstruction/NvidiaDlssRayReconstructionProvider.h"

#include "RayReconstruction/NvidiaDlssRayReconstruction/StreamlineRayReconstructionEvaluation.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	#include "Streamline/StreamlineRuntimeSupport.h"

	#include <sl.h>
	#include <sl_dlss.h>
	#include <sl_dlss_d.h>
#endif

namespace
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	constexpr std::uint32_t kRayReconstructionViewportId = 2u;
#endif
}

bool NvidiaDlssRayReconstructionProvider::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop)
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	m_initialized = IsStreamlineFeatureSupported(sl::kFeatureDLSS_RR, capabilities, nativeInterop);
	return m_initialized;
#else
	(void) capabilities;
	(void) nativeInterop;
	return false;
#endif
}

RenderViewportExtent NvidiaDlssRayReconstructionProvider::ResolveRenderExtent(RenderViewportExtent outputExtent) noexcept
{
	RenderViewportExtent providerRenderExtent = {};
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	if (m_initialized)
	{
		providerRenderExtent = QueryStreamlineRayReconstructionOptimalRenderExtent(
		    outputExtent,
		    m_frameState.GetRequestedQualityMode());
	}
#endif
	return m_frameState.StoreResolution(outputExtent, providerRenderExtent);
}

void NvidiaDlssRayReconstructionProvider::SetupFrame(const ImageProviderFrameContext& frameContext)
{
	m_frameState.SetupFrame(frameContext);
}

bool NvidiaDlssRayReconstructionProvider::Evaluate(const RayReconstructionEvaluationDesc& evaluation)
{
	if (!m_initialized || !m_frameState.IsValid())
	{
		return false;
	}

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	return EvaluateStreamlineRayReconstructionFrame(
	    m_frameState.GetFrameContext(),
	    m_frameState.GetQualityMode(),
	    sl::ViewportHandle{kRayReconstructionViewportId},
	    evaluation);
#else
	(void) evaluation;
	return false;
#endif
}

void NvidiaDlssRayReconstructionProvider::Shutdown() noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	if (m_initialized)
	{
		(void) slFreeResources(sl::kFeatureDLSS_RR, sl::ViewportHandle{kRayReconstructionViewportId});
		(void) slFreeResources(sl::kFeatureDLSS, sl::ViewportHandle{kRayReconstructionViewportId});
	}
#endif
	m_initialized = false;
	m_frameState.Reset();
}
