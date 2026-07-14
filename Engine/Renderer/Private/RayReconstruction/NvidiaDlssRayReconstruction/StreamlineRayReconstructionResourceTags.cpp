#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlssRayReconstruction/StreamlineRayReconstructionResourceTags.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineResourceInterop.h"

#include <array>

sl::Result TagRayReconstructionResourcesForFrame(
    const sl::FrameToken& frameToken,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation) noexcept
{
	sl::Extent renderExtent = BuildStreamlineExtent(evaluation.RenderExtent);
	sl::Extent outputExtent = BuildStreamlineExtent(evaluation.OutputExtent);
	sl::Extent exposureExtent = BuildStreamlineExtent(RenderViewportExtent{1u, 1u});
	StreamlineTaggedTextureResource noisyColor(
	    evaluation.BackendApi,
	    evaluation.NativeNoisyInputColorView);
	StreamlineTaggedTextureResource outputColor(
	    evaluation.BackendApi,
	    evaluation.NativeOutputColorView);
	StreamlineTaggedTextureResource depth(
	    evaluation.BackendApi,
	    evaluation.NativeDepthView);
	StreamlineTaggedTextureResource motionVectors(
	    evaluation.BackendApi,
	    evaluation.NativeMotionVectorsView);
	StreamlineTaggedTextureResource normals(
	    evaluation.BackendApi,
	    evaluation.NativeNormalsView);
	StreamlineTaggedTextureResource roughness(
	    evaluation.BackendApi,
	    evaluation.NativeRoughnessView);
	StreamlineTaggedTextureResource diffuseAlbedo(
	    evaluation.BackendApi,
	    evaluation.NativeDiffuseAlbedoView);
	StreamlineTaggedTextureResource specularAlbedo(
	    evaluation.BackendApi,
	    evaluation.NativeSpecularAlbedoView);
	StreamlineTaggedTextureResource specularHitDistance(
	    evaluation.BackendApi,
	    evaluation.NativeSpecularHitDistanceView);
	StreamlineTaggedTextureResource exposure(
	    evaluation.BackendApi,
	    evaluation.NativeExposureView);

	std::array<sl::ResourceTag, 10> tags = {
	    sl::ResourceTag{noisyColor.Get(), sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{outputColor.Get(), sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate, &outputExtent},
	    sl::ResourceTag{depth.Get(), sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{motionVectors.Get(), sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{diffuseAlbedo.Get(), sl::kBufferTypeAlbedo, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{specularAlbedo.Get(), sl::kBufferTypeSpecularAlbedo, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{normals.Get(), sl::kBufferTypeNormals, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{roughness.Get(), sl::kBufferTypeRoughness, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{specularHitDistance.Get(), sl::kBufferTypeSpecularHitDistance, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
	    sl::ResourceTag{exposure.Get(), sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilEvaluate, &exposureExtent}};

	auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
	return slSetTagForFrame(frameToken, viewport, tags.data(), static_cast<std::uint32_t>(tags.size()), commandBuffer);
}
#endif
