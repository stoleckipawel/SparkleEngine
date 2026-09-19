#include "../../PCH.h"
#include "Passes/Lighting/LightingTargetClear.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "FrameGraph/ResourceUsage.h"

#include <array>

static constexpr const char* LightingTargetClearPassName = "LightingTargetClear";

static auto GetLightingTargets(const LightingRenderTargets& lighting) noexcept
{
	return std::array{
	    lighting.DirectDiffuse,
	    lighting.DirectSpecular,
	    lighting.DirectSubsurface,
	    lighting.IndirectDiffuse,
	    lighting.IndirectSpecular};
}

static auto GetRayReconstructionGuideTargets(const LightingRenderTargets& lighting) noexcept
{
	return std::array{
	    lighting.ReconstructionGuides.DiffuseAlbedo,
	    lighting.ReconstructionGuides.SpecularAlbedo,
	    lighting.ReconstructionGuides.Roughness,
	    lighting.ReconstructionGuides.SpecularHitDistance};
}

void AddLightingTargetClearPass(FrameGraphBuilder& builder, const RenderFrameGraphResources& resources)
{
	const LightingRenderTargets& lighting = resources.Transient.Lighting;

	builder.AddPass(
	    LightingTargetClearPassName,
	    EFrameGraphPassKind::Raster,
	    [lighting](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Write(lighting.DirectDiffuse, ResourceUsage::RenderTarget, "DirectDiffuse");
		    resourceBuilder.Write(lighting.DirectSpecular, ResourceUsage::RenderTarget, "DirectSpecular");
		    resourceBuilder.Write(lighting.DirectSubsurface, ResourceUsage::RenderTarget, "DirectSubsurface");
		    resourceBuilder.Write(lighting.IndirectDiffuse, ResourceUsage::RenderTarget, "IndirectDiffuse");
		    resourceBuilder.Write(lighting.IndirectSpecular, ResourceUsage::RenderTarget, "IndirectSpecular");

		    resourceBuilder.Write(
		        lighting.ReconstructionGuides.DiffuseAlbedo,
		        ResourceUsage::RenderTarget,
		        "RayReconstructionDiffuseAlbedo");
		    resourceBuilder.Write(
		        lighting.ReconstructionGuides.SpecularAlbedo,
		        ResourceUsage::RenderTarget,
		        "RayReconstructionSpecularAlbedo");
		    resourceBuilder.Write(lighting.ReconstructionGuides.Roughness, ResourceUsage::RenderTarget, "RayReconstructionRoughness");
		    resourceBuilder.Write(
		        lighting.ReconstructionGuides.SpecularHitDistance,
		        ResourceUsage::RenderTarget,
		        "RayReconstructionSpecularHitDistance");
	    },
	    [lighting](PassCommandContext& context)
	    {
		    for (FrameGraphTextureHandle target : GetLightingTargets(lighting))
		    {
			    context.Resources.ClearRenderTarget(context.Commands, target);
		    }

		    for (FrameGraphTextureHandle target : GetRayReconstructionGuideTargets(lighting))
		    {
			    context.Resources.ClearRenderTarget(context.Commands, target);
		    }
	    });
}
