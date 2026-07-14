#include "../../PCH.h"
#include "Frame/Lighting/RestirLightingInvalidation.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Lighting/LightingSceneState.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"

std::uint64_t BuildRestirLightingHistoryInvalidationHash(const FrameContext& frame) noexcept
{
	const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
	std::uint64_t hash = BuildLightingSceneInvalidationHash(frame);
	hash = Hash::ContinueFnv1a64Value(hash, settings.BounceCount);
	hash = Hash::ContinueFnv1a64Value(hash, settings.NormalBias);
	hash = Hash::ContinueFnv1a64Value(hash, settings.MaxDistance);
	return Hash::FinalizeFnv1a64(hash);
}
