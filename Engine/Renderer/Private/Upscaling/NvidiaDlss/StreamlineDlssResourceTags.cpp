#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssResourceTags.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineResourceInterop.h"

#include <array>

sl::Result TagDlssResourcesForFrame(
    const sl::FrameToken& frameToken,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation) noexcept
{
	sl::Extent renderExtent = BuildStreamlineExtent(evaluation.RenderExtent);
	sl::Extent outputExtent = BuildStreamlineExtent(evaluation.OutputExtent);
	sl::Extent exposureExtent = BuildStreamlineExtent(RenderViewportExtent{1u, 1u});
	StreamlineTaggedTextureResource colorIn(
	    evaluation.BackendApi,
	    evaluation.NativeScalingInputColorView,
	    StreamlineTextureAccess::ReadOnly);
	StreamlineTaggedTextureResource colorOut(
	    evaluation.BackendApi,
	    evaluation.NativeScalingOutputColorView,
	    StreamlineTextureAccess::ReadWrite);
	StreamlineTaggedTextureResource depth(
	    evaluation.BackendApi,
	    evaluation.NativeDepthView,
	    StreamlineTextureAccess::ReadOnly);
	StreamlineTaggedTextureResource motionVectors(
	    evaluation.BackendApi,
	    evaluation.NativeMotionVectorsView,
	    StreamlineTextureAccess::ReadOnly);
	StreamlineTaggedTextureResource exposure(
	    evaluation.BackendApi,
	    evaluation.NativeExposureView,
	    StreamlineTextureAccess::ReadOnly);

	std::array<sl::ResourceTag, 5> tags = {
	    sl::ResourceTag{colorIn.Get(), sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{colorOut.Get(), sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &outputExtent},
	    sl::ResourceTag{depth.Get(), sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{motionVectors.Get(), sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{exposure.Get(), sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilEvaluate, &exposureExtent}};

	auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
	return slSetTagForFrame(frameToken, viewport, tags.data(), static_cast<std::uint32_t>(tags.size()), commandBuffer);
}
#endif
