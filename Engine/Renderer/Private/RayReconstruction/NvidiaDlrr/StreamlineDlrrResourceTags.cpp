#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlrr/StreamlineDlrrResourceTags.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineResourceInterop.h"

#include <array>

namespace
{
	constexpr std::uint32_t kD3D12ResourceStateUnorderedAccess = 0x00000008u;
	constexpr std::uint32_t kD3D12ResourceStateDepthRead = 0x00000020u;
	constexpr std::uint32_t kD3D12ResourceStateNonPixelShaderResource = 0x00000040u;
	constexpr std::uint32_t kD3D12ResourceStatePixelShaderResource = 0x00000080u;
	constexpr std::uint32_t kD3D12ShaderResourceState =
	    kD3D12ResourceStateNonPixelShaderResource | kD3D12ResourceStatePixelShaderResource;
}

sl::Result TagDlrrResourcesForFrame(
    const sl::FrameToken& frameToken,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation) noexcept
{
	sl::Extent renderExtent{.top = 0, .left = 0, .width = evaluation.RenderExtent.Width, .height = evaluation.RenderExtent.Height};
	sl::Extent outputExtent{.top = 0, .left = 0, .width = evaluation.OutputExtent.Width, .height = evaluation.OutputExtent.Height};

	sl::Resource noisyColor = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeNoisyInputColor,
	    evaluation.NativeNoisyInputColorView,
	    kD3D12ShaderResourceState);
	sl::Resource outputColor = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeOutputColor,
	    evaluation.NativeOutputColorView,
	    kD3D12ResourceStateUnorderedAccess);
	sl::Resource depth =
	    BuildStreamlineTextureResource(evaluation.BackendApi, evaluation.NativeDepth, evaluation.NativeDepthView, kD3D12ResourceStateDepthRead);
	sl::Resource motionVectors = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeMotionVectors,
	    evaluation.NativeMotionVectorsView,
	    kD3D12ShaderResourceState);
	sl::Resource normals =
	    BuildStreamlineTextureResource(evaluation.BackendApi, evaluation.NativeNormals, evaluation.NativeNormalsView, kD3D12ShaderResourceState);
	sl::Resource roughness = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeRoughness,
	    evaluation.NativeRoughnessView,
	    kD3D12ShaderResourceState);
	sl::Resource diffuseAlbedo = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeDiffuseAlbedo,
	    evaluation.NativeDiffuseAlbedoView,
	    kD3D12ShaderResourceState);
	sl::Resource specularAlbedo = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeSpecularAlbedo,
	    evaluation.NativeSpecularAlbedoView,
	    kD3D12ShaderResourceState);
	sl::Resource specularHitDistance = BuildStreamlineTextureResource(
	    evaluation.BackendApi,
	    evaluation.NativeSpecularHitDistance,
	    evaluation.NativeSpecularHitDistanceView,
	    kD3D12ShaderResourceState);
	sl::Resource exposure =
	    BuildStreamlineTextureResource(evaluation.BackendApi, evaluation.NativeExposure, evaluation.NativeExposureView, kD3D12ShaderResourceState);

	sl::SubresourceRange noisyColorRange = BuildStreamlineSubresourceRange(evaluation.NativeNoisyInputColorView);
	sl::SubresourceRange outputColorRange = BuildStreamlineSubresourceRange(evaluation.NativeOutputColorView);
	sl::SubresourceRange depthRange = BuildStreamlineSubresourceRange(evaluation.NativeDepthView);
	sl::SubresourceRange motionVectorsRange = BuildStreamlineSubresourceRange(evaluation.NativeMotionVectorsView);
	sl::SubresourceRange normalsRange = BuildStreamlineSubresourceRange(evaluation.NativeNormalsView);
	sl::SubresourceRange roughnessRange = BuildStreamlineSubresourceRange(evaluation.NativeRoughnessView);
	sl::SubresourceRange diffuseAlbedoRange = BuildStreamlineSubresourceRange(evaluation.NativeDiffuseAlbedoView);
	sl::SubresourceRange specularAlbedoRange = BuildStreamlineSubresourceRange(evaluation.NativeSpecularAlbedoView);
	sl::SubresourceRange specularHitDistanceRange = BuildStreamlineSubresourceRange(evaluation.NativeSpecularHitDistanceView);
	sl::SubresourceRange exposureRange = BuildStreamlineSubresourceRange(evaluation.NativeExposureView);
	if (evaluation.BackendApi == ERhiBackendApi::Vulkan)
	{
		noisyColor.next = &noisyColorRange;
		outputColor.next = &outputColorRange;
		depth.next = &depthRange;
		motionVectors.next = &motionVectorsRange;
		normals.next = &normalsRange;
		roughness.next = &roughnessRange;
		diffuseAlbedo.next = &diffuseAlbedoRange;
		specularAlbedo.next = &specularAlbedoRange;
		specularHitDistance.next = &specularHitDistanceRange;
		exposure.next = &exposureRange;
	}

	std::array<sl::ResourceTag, 10> tags = {
	    sl::ResourceTag{&noisyColor, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&outputColor, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &outputExtent},
	    sl::ResourceTag{&depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&motionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&diffuseAlbedo, sl::kBufferTypeAlbedo, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&specularAlbedo, sl::kBufferTypeSpecularAlbedo, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&normals, sl::kBufferTypeNormals, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&roughness, sl::kBufferTypeRoughness, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&specularHitDistance, sl::kBufferTypeSpecularHitDistance, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{&exposure, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent}};

	auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
	return slSetTagForFrame(frameToken, viewport, tags.data(), static_cast<std::uint32_t>(tags.size()), commandBuffer);
}
#endif
