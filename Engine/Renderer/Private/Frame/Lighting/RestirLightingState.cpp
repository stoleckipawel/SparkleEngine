#include "../../PCH.h"
#include "Frame/Lighting/RestirLightingState.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Lighting/LightingCVars.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"

std::uint64_t BuildRestirLightingSettingsKey() noexcept
{
	const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
	std::uint64_t hash = Hash::kFnv64OffsetBasis;
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
	return Hash::FinalizeFnv1a64(hash);
}
