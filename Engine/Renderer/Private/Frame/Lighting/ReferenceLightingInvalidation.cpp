#include "../../PCH.h"
#include "Frame/Lighting/ReferenceLightingInvalidation.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Frame/Lighting/LightingSceneState.h"
#include "Frame/Lighting/LightingStateHash.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "View/RenderView.h"

std::uint64_t BuildReferenceLightingHistoryInvalidationHash(const PreparedRenderScene& scene, const RenderView& view) noexcept
{
	const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
	std::uint64_t hash = BuildLightingSceneInvalidationHash(scene);
	hash = Hash::ContinueFnv1a64Value(hash, settings.SamplesPerPixel);
	hash = Hash::ContinueFnv1a64Value(hash, settings.BounceCount);
	hash = Hash::ContinueFnv1a64Value(hash, settings.NormalBias);
	hash = Hash::ContinueFnv1a64Value(hash, settings.MaxDistance);
	hash = Hash::ContinueFnv1a64Value(hash, CVarRenderViewMode.Get());
	const ViewCameraUniformData& camera = view.cameraUniform;
	hash = LightingStateHash::AppendMatrix(hash, camera.ViewMTX);
	hash = LightingStateHash::AppendMatrix(hash, camera.ProjectionMTX);
	return Hash::FinalizeFnv1a64(hash);
}
