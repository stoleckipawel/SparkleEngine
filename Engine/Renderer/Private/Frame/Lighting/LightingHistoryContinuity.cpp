#include "PCH.h"
#include "Frame/Lighting/LightingHistoryContinuity.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Lighting/LightingSceneState.h"
#include "Frame/Lighting/LightingStateHash.h"
#include "Lighting/LightingCVars.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "Renderer/Public/Debug/RendererCVars.h"

namespace
{
	std::uint64_t AppendSharedLightingState(std::uint64_t hash, const FrameContext& frame) noexcept
	{
		hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowsEnabled.Get());
		hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowNormalBias.Get());
		hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowMaxDistance.Get());
		hash = Hash::ContinueFnv1a64Value(hash, CVarMaxDirectionalLights.Get());
		hash = Hash::ContinueFnv1a64Value(hash, CVarMaxPointLights.Get());
		hash = Hash::ContinueFnv1a64Value(hash, CVarMaxSpotLights.Get());
		hash = Hash::ContinueFnv1a64Value(hash, CVarMaxRectLights.Get());
		return Hash::ContinueFnv1a64Value(hash, BuildLightingSceneStateKey(frame));
	}

	std::uint64_t BuildReferenceContinuity(const FrameContext& frame) noexcept
	{
		const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
		std::uint64_t hash = Hash::kFnv64OffsetBasis;
		hash = Hash::ContinueFnv1a64Value(hash, settings.SamplesPerPixel);
		hash = Hash::ContinueFnv1a64Value(hash, settings.BounceCount);
		hash = Hash::ContinueFnv1a64Value(hash, settings.NormalBias);
		hash = Hash::ContinueFnv1a64Value(hash, settings.MaxDistance);
		hash = Hash::ContinueFnv1a64Value(hash, CVarRenderViewMode.Get());
		const PerViewCameraConstantBufferData& camera = frame.mainView.perViewData.Camera;
		hash = LightingStateHash::AppendMatrix(hash, camera.ViewMTX);
		hash = LightingStateHash::AppendMatrix(hash, camera.ProjectionMTX);
		return Hash::FinalizeFnv1a64(AppendSharedLightingState(hash, frame));
	}

	std::uint64_t BuildRestirContinuity(const FrameContext& frame) noexcept
	{
		const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
		std::uint64_t hash = Hash::kFnv64OffsetBasis;
		hash = Hash::ContinueFnv1a64Value(hash, settings.BounceCount);
		hash = Hash::ContinueFnv1a64Value(hash, settings.NormalBias);
		hash = Hash::ContinueFnv1a64Value(hash, settings.MaxDistance);
		return Hash::FinalizeFnv1a64(AppendSharedLightingState(hash, frame));
	}
}

LightingHistoryContinuity BuildLightingHistoryContinuity(const FrameContext& frame) noexcept
{
	return LightingHistoryContinuity{
	    .ReferenceLighting = BuildReferenceContinuity(frame),
	    .RestirLighting = BuildRestirContinuity(frame)};
}
