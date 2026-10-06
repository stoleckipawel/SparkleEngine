#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingResources.h"

#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/LightingRenderTargets.h"
#include "Passes/Lighting/LightingTargetClear.h"

#include <array>

void CreateIndirectLightingResources(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	CreateIndirectLightingRenderTargets(builder, sceneExtent, resources);
	const auto& lighting = resources.Transient.Lighting;
	const std::array indirectTargets{lighting.IndirectDiffuse, lighting.IndirectSpecular};
	AddLightingTargetClearPass(builder, "IndirectLightingTargetClear", sceneExtent, indirectTargets);
	if (IsIndirectLightingAdmitted())
	{
		const auto& guides = lighting.ReconstructionGuides;
		const std::array guideTargets{guides.DiffuseAlbedo, guides.SpecularAlbedo, guides.Roughness, guides.SpecularHitDistance};
		AddLightingTargetClearPass(builder, "RayReconstructionGuideClear", sceneExtent, guideTargets);
	}
}
