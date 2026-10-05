#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingResources.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/LightingRenderTargets.h"
#include "Passes/Lighting/LightingTargetClear.h"

#include <array>

void CreateIndirectLightingResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool createRayReconstructionGuides,
    RenderFrameGraphResources& resources)
{
	CreateIndirectLightingRenderTargets(builder, sceneExtent, createRayReconstructionGuides, resources);
	const auto& lighting = resources.Transient.Lighting;
	const std::array indirectTargets{lighting.IndirectDiffuse, lighting.IndirectSpecular};
	AddLightingTargetClearPass(builder, "IndirectLightingTargetClear", sceneExtent, indirectTargets);
	if (createRayReconstructionGuides)
	{
		const auto& guides = lighting.ReconstructionGuides;
		const std::array guideTargets{guides.DiffuseAlbedo, guides.SpecularAlbedo, guides.Roughness, guides.SpecularHitDistance};
		AddLightingTargetClearPass(builder, "RayReconstructionGuideClear", sceneExtent, guideTargets);
	}
}
