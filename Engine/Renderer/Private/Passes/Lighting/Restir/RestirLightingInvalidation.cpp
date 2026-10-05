#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirLightingInvalidation.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Passes/Lighting/LightingSceneState.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "Passes/Lighting/Shadows/DirectShadowControls.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"

std::uint64_t BuildRestirLightingHistoryInvalidationHash(const PreparedRenderScene& scene) noexcept
{
	const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
	std::uint64_t hash = BuildLightingSceneInvalidationHash(scene);
	hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowNormalBias.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowMaxDistance.Get());
	hash = Hash::ContinueFnv1a64Value(hash, settings.BounceCount);
	hash = Hash::ContinueFnv1a64Value(hash, CVarRestirIndirectTemporalReuse.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarRestirIndirectSpatialReuse.Get());
	hash = AppendDirectLightingHistoryInvalidationHash(hash);
	hash = AppendIndirectLightingHistoryInvalidationHash(hash);
	hash = AppendDirectShadowHistoryInvalidationHash(hash);
	return Hash::FinalizeFnv1a64(hash);
}
