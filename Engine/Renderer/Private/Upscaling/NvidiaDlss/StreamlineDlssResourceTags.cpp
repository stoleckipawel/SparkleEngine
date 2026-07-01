#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssResourceTags.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <array>

namespace
{
	constexpr std::uint32_t kD3D12ResourceStateUnorderedAccess = 0x00000008u;
	constexpr std::uint32_t kD3D12ResourceStateDepthRead = 0x00000020u;
	constexpr std::uint32_t kD3D12ResourceStateNonPixelShaderResource = 0x00000040u;
	constexpr std::uint32_t kD3D12ResourceStatePixelShaderResource = 0x00000080u;
	constexpr std::uint32_t kD3D12ResourceStateCopySource = 0x00000800u;

	sl::SubresourceRange BuildStreamlineSubresourceRange(const NativeTextureViewInfo& view) noexcept
	{
		sl::SubresourceRange range{};
		range.aspectMask = view.SubresourceAspectMask;
		range.baseMipLevel = view.SubresourceBaseMipLevel;
		range.levelCount = view.SubresourceLevelCount;
		range.baseArrayLayer = view.SubresourceBaseArrayLayer;
		range.layerCount = view.SubresourceLayerCount;
		return range;
	}

	sl::Resource BuildVulkanStreamlineTextureResource(const NativeTextureViewInfo& view) noexcept
	{
		sl::Resource resource{
		    sl::ResourceType::eTex2d,
		    view.Resource.Value,
		    nullptr,
		    view.View.Value,
		    view.NativeState};
		resource.width = view.Width;
		resource.height = view.Height;
		resource.nativeFormat = view.NativeFormat;
		resource.mipLevels = view.MipLevels;
		resource.arrayLayers = view.ArrayLayers;
		resource.flags = view.NativeFlags;
		resource.usage = view.NativeUsage;
		return resource;
	}
}

sl::Result TagDlssResourcesForFrame(
    const sl::FrameToken& frameToken,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation) noexcept
{
	sl::Extent renderExtent{.top = 0, .left = 0, .width = evaluation.RenderExtent.Width, .height = evaluation.RenderExtent.Height};
	sl::Extent outputExtent{.top = 0, .left = 0, .width = evaluation.OutputExtent.Width, .height = evaluation.OutputExtent.Height};
	sl::Resource colorIn = evaluation.BackendApi == ERhiBackendApi::Vulkan ?
	                           BuildVulkanStreamlineTextureResource(evaluation.NativeScalingInputColorView) :
	                           sl::Resource{sl::ResourceType::eTex2d, evaluation.NativeScalingInputColor.Value, kD3D12ResourceStateCopySource};
	sl::Resource colorOut =
	    evaluation.BackendApi == ERhiBackendApi::Vulkan ?
	        BuildVulkanStreamlineTextureResource(evaluation.NativeScalingOutputColorView) :
	        sl::Resource{sl::ResourceType::eTex2d, evaluation.NativeScalingOutputColor.Value, kD3D12ResourceStateUnorderedAccess};
	sl::Resource depth = evaluation.BackendApi == ERhiBackendApi::Vulkan ?
	                         BuildVulkanStreamlineTextureResource(evaluation.NativeDepthView) :
	                         sl::Resource{sl::ResourceType::eTex2d, evaluation.NativeDepth.Value, kD3D12ResourceStateDepthRead};
	sl::Resource motionVectors =
	    evaluation.BackendApi == ERhiBackendApi::Vulkan ?
	        BuildVulkanStreamlineTextureResource(evaluation.NativeMotionVectorsView) :
	        sl::Resource{
	            sl::ResourceType::eTex2d,
	            evaluation.NativeMotionVectors.Value,
	            kD3D12ResourceStateNonPixelShaderResource | kD3D12ResourceStatePixelShaderResource};
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
