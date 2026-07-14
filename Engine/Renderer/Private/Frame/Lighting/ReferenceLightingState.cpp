#include "../../PCH.h"
#include "Frame/Lighting/ReferenceLightingState.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Lighting/LightingSceneState.h"
#include "Frame/Lighting/LightingStateHash.h"
#include "Lighting/LightingCVars.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <cstdint>

std::uint64_t BuildReferenceLightingHistoryKey(const FrameContext& frame) noexcept
{
	const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
	std::uint64_t hash = Hash::kFnv64OffsetBasis;
	hash = Hash::ContinueFnv1a64Value(hash, settings.SamplesPerPixel);
	hash = Hash::ContinueFnv1a64Value(hash, settings.BounceCount);
	hash = Hash::ContinueFnv1a64Value(hash, settings.NormalBias);
	hash = Hash::ContinueFnv1a64Value(hash, settings.MaxDistance);
	hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowsEnabled.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowNormalBias.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowMaxDistance.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarMaxDirectionalLights.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarMaxPointLights.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarMaxSpotLights.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarMaxRectLights.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarRenderViewMode.Get());
	const PerViewCameraConstantBufferData& camera = frame.mainView.perViewData.Camera;
	hash = LightingStateHash::AppendMatrix(hash, camera.ViewMTX);
	hash = LightingStateHash::AppendMatrix(hash, camera.ProjectionMTX);
	hash = Hash::ContinueFnv1a64Value(hash, BuildLightingSceneStateKey(frame));
	return Hash::FinalizeFnv1a64(hash);
}
