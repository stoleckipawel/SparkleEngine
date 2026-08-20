#include "../../PCH.h"
#include "Frame/Lighting/LightingTargetClear.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "FrameGraph/ResourceUsage.h"

#include <array>

class LightingTargetClearPlan final
{
public:
	static constexpr const char* kLightingTargetClearPassName = "LightingTargetClear";

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
};

void AddLightingTargetClearPass(FrameGraphBuilder& builder, const LightingRenderTargets& lighting)
{
	builder.AddPass(
	    LightingTargetClearPlan::kLightingTargetClearPassName,
	    EFrameGraphPassKind::Raster,
	    [lighting](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Write(lighting.DirectDiffuse, ResourceUsage::RenderTarget, "DirectDiffuse");
		    resourceBuilder.Write(lighting.DirectSpecular, ResourceUsage::RenderTarget, "DirectSpecular");
		    resourceBuilder.Write(lighting.DirectSubsurface, ResourceUsage::RenderTarget, "DirectSubsurface");
		    resourceBuilder.Write(lighting.IndirectDiffuse, ResourceUsage::RenderTarget, "IndirectDiffuse");
		    resourceBuilder.Write(lighting.IndirectSpecular, ResourceUsage::RenderTarget, "IndirectSpecular");
		    if (lighting.ReconstructionGuides.IsValid())
		    {
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
		    }
	    },
	    [lighting](PassCommandContext& context)
	    {
		    for (FrameGraphTextureHandle target : LightingTargetClearPlan::GetLightingTargets(lighting))
		    {
			    context.Resources.ClearRenderTarget(context.Commands, target);
		    }
		    if (lighting.ReconstructionGuides.IsValid())
		    {
			    for (FrameGraphTextureHandle target : LightingTargetClearPlan::GetRayReconstructionGuideTargets(lighting))
			    {
				    context.Resources.ClearRenderTarget(context.Commands, target);
			    }
		    }
	    });
}
