#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssResourceTags.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineResourceInterop.h"

#include <array>

namespace
{
	constexpr std::uint32_t kD3D12ResourceStateUnorderedAccess = 0x00000008u;
	constexpr std::uint32_t kD3D12ResourceStateNonPixelShaderResource = 0x00000040u;
	constexpr std::uint32_t kD3D12ResourceStatePixelShaderResource = 0x00000080u;
	constexpr std::uint32_t kD3D12ShaderResourceState =
	    kD3D12ResourceStateNonPixelShaderResource | kD3D12ResourceStatePixelShaderResource;

}

sl::Result TagDlssResourcesForFrame(
    const sl::FrameToken& frameToken,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation) noexcept
{
	sl::Extent renderExtent{.top = 0, .left = 0, .width = evaluation.RenderExtent.Width, .height = evaluation.RenderExtent.Height};
	sl::Extent outputExtent{.top = 0, .left = 0, .width = evaluation.OutputExtent.Width, .height = evaluation.OutputExtent.Height};
	sl::Resource colorIn = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeScalingInputColor,
	    evaluation.NativeScalingInputColorView,
	    kD3D12ShaderResourceState);
	sl::Resource colorOut = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeScalingOutputColor,
	    evaluation.NativeScalingOutputColorView,
	    kD3D12ResourceStateUnorderedAccess);
	sl::Resource depth =
	    BuildStreamlineTextureResource(evaluation.BackendApi, evaluation.NativeDepth, evaluation.NativeDepthView, kD3D12ShaderResourceState);
	sl::Resource motionVectors = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeMotionVectors,
	    evaluation.NativeMotionVectorsView,
	    kD3D12ShaderResourceState);
	sl::SubresourceRange colorInRange = BuildStreamlineSubresourceRange(evaluation.NativeScalingInputColorView);
	sl::SubresourceRange colorOutRange = BuildStreamlineSubresourceRange(evaluation.NativeScalingOutputColorView);
	sl::SubresourceRange depthRange = BuildStreamlineSubresourceRange(evaluation.NativeDepthView);
	sl::SubresourceRange motionVectorsRange = BuildStreamlineSubresourceRange(evaluation.NativeMotionVectorsView);
	if (evaluation.BackendApi == ERhiBackendApi::Vulkan)
	{
		colorIn.next = &colorInRange;
		colorOut.next = &colorOutRange;
		depth.next = &depthRange;
		motionVectors.next = &motionVectorsRange;
	}

	std::array<sl::ResourceTag, 4> tags = {
	    sl::ResourceTag{&colorIn, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&colorOut, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &outputExtent},
	    sl::ResourceTag{&depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&motionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent}};

	auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
	return slSetTagForFrame(frameToken, viewport, tags.data(), static_cast<std::uint32_t>(tags.size()), commandBuffer);
}
#endif
