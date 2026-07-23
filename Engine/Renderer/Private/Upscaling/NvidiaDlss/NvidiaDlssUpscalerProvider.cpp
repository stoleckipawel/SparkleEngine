#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h"

#include "Upscaling/NvidiaDlss/StreamlineDlssEvaluation.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	#include "Streamline/StreamlineRuntimeSupport.h"

	#include <sl.h>
	#include <sl_dlss.h>
#endif

class NvidiaDlssUpscalerProviderConstants final
{
  public:
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	static constexpr std::uint32_t kDlssViewportId = 1u;
#endif
};

bool NvidiaDlssUpscalerProvider::Initialize(const RhiCapabilities& capabilities, RhiNativeDeviceQueueInterop nativeInterop)
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	m_initialized = IsStreamlineFeatureSupported(sl::kFeatureDLSS, capabilities, nativeInterop);
	return m_initialized;
#else
	(void) capabilities;
	(void) nativeInterop;
	return false;
#endif
}

RenderViewportExtent NvidiaDlssUpscalerProvider::ResolveRenderExtent(RenderViewportExtent outputExtent) noexcept
{
	RenderViewportExtent providerRenderExtent = {};
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	if (m_initialized)
	{
		providerRenderExtent =
		    QueryStreamlineDlssOptimalRenderExtent(outputExtent, m_frameState.GetRequestedQualityMode());
	}
#endif
	return m_frameState.StoreResolution(outputExtent, providerRenderExtent);
}

void NvidiaDlssUpscalerProvider::SetupFrame(const ImageProviderFrameContext& frameContext)
{
	m_frameState.SetupFrame(frameContext);
}

bool NvidiaDlssUpscalerProvider::Evaluate(const UpscalerEvaluationDesc& evaluation)
{
	if (!m_initialized || !m_frameState.IsValid())
	{
		return false;
	}

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	return EvaluateStreamlineDlssFrame(
	    m_frameState.GetFrameContext(),
	    m_frameState.GetQualityMode(),
	    sl::ViewportHandle{NvidiaDlssUpscalerProviderConstants::kDlssViewportId},
	    evaluation);
#else
	(void) evaluation;
	return false;
#endif
}

void NvidiaDlssUpscalerProvider::Shutdown() noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	if (m_initialized)
	{
		(void) slFreeResources(sl::kFeatureDLSS, sl::ViewportHandle{NvidiaDlssUpscalerProviderConstants::kDlssViewportId});
	}
#endif
	m_initialized = false;
	m_frameState.Reset();
}
