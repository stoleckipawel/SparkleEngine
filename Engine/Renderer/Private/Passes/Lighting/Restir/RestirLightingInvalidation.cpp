#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirLightingInvalidation.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Passes/Lighting/LightingSceneState.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"

std::uint64_t BuildRestirLightingHistoryInvalidationHash(const PreparedRenderScene& scene) noexcept
{
	const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
	std::uint64_t hash = BuildLightingSceneInvalidationHash(scene);
	hash = Hash::ContinueFnv1a64Value(hash, settings.BounceCount);
	hash = Hash::ContinueFnv1a64Value(hash, settings.NormalBias);
	hash = Hash::ContinueFnv1a64Value(hash, settings.MaxDistance);
	return Hash::FinalizeFnv1a64(hash);
}
