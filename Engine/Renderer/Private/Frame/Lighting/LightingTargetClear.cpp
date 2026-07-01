#include "../../PCH.h"
#include "Frame/Lighting/LightingTargetClear.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/ResourceUsage.h"

#include <array>

namespace
{
	constexpr const char* kLightingTargetClearPassName = "LightingTargetClear";

	auto GetLightingTargets(const LightingRenderTargets& lighting) noexcept
	{
		return std::array{
		    lighting.DirectDiffuse,
		    lighting.DirectSpecular,
		    lighting.DirectSubsurface,
		    lighting.IndirectDiffuse,
		    lighting.IndirectSpecular,
		    lighting.IndirectSubsurface,
		    lighting.IndirectDiffuseDemodulatedRadiance,
		    lighting.IndirectSpecularDemodulatedRadiance,
		    lighting.IndirectDiffuseAlbedo,
		    lighting.IndirectSpecularAlbedo,
		    lighting.IndirectMaterialGuide,
		    lighting.IndirectDiffuseSampleGuide,
		    lighting.IndirectSpecularSampleGuide,
		    lighting.ReferenceDirect,
		    lighting.ReferenceIndirectDiffuse,
		    lighting.ReferenceIndirectSpecular,
		    lighting.ReferenceSceneColor};
	}
}

void AddLightingTargetClearPass(FrameGraphBuilder& builder, const LightingRenderTargets& lighting)
{
	builder.AddPass(
	    kLightingTargetClearPassName,
	    EFrameGraphPassFlags::Raster,
	    [lighting](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Write(lighting.DirectDiffuse, ResourceUsage::RenderTarget, "DirectDiffuse");
		    resourceBuilder.Write(lighting.DirectSpecular, ResourceUsage::RenderTarget, "DirectSpecular");
		    resourceBuilder.Write(lighting.DirectSubsurface, ResourceUsage::RenderTarget, "DirectSubsurface");
		    resourceBuilder.Write(lighting.IndirectDiffuse, ResourceUsage::RenderTarget, "IndirectDiffuse");
		    resourceBuilder.Write(lighting.IndirectSpecular, ResourceUsage::RenderTarget, "IndirectSpecular");
		    resourceBuilder.Write(lighting.IndirectSubsurface, ResourceUsage::RenderTarget, "IndirectSubsurface");
		    resourceBuilder.Write(
		        lighting.IndirectDiffuseDemodulatedRadiance,
		        ResourceUsage::RenderTarget,
		        "IndirectDiffuseDemodulatedRadiance");
		    resourceBuilder.Write(
		        lighting.IndirectSpecularDemodulatedRadiance,
		        ResourceUsage::RenderTarget,
		        "IndirectSpecularDemodulatedRadiance");
		    resourceBuilder.Write(lighting.IndirectDiffuseAlbedo, ResourceUsage::RenderTarget, "IndirectDiffuseAlbedo");
		    resourceBuilder.Write(lighting.IndirectSpecularAlbedo, ResourceUsage::RenderTarget, "IndirectSpecularAlbedo");
		    resourceBuilder.Write(lighting.IndirectMaterialGuide, ResourceUsage::RenderTarget, "IndirectMaterialGuide");
		    resourceBuilder.Write(lighting.IndirectDiffuseSampleGuide, ResourceUsage::RenderTarget, "IndirectDiffuseSampleGuide");
		    resourceBuilder.Write(lighting.IndirectSpecularSampleGuide, ResourceUsage::RenderTarget, "IndirectSpecularSampleGuide");
		    resourceBuilder.Write(lighting.ReferenceDirect, ResourceUsage::RenderTarget, "ReferenceDirect");
		    resourceBuilder.Write(lighting.ReferenceIndirectDiffuse, ResourceUsage::RenderTarget, "ReferenceIndirectDiffuse");
		    resourceBuilder.Write(lighting.ReferenceIndirectSpecular, ResourceUsage::RenderTarget, "ReferenceIndirectSpecular");
		    resourceBuilder.Write(lighting.ReferenceSceneColor, ResourceUsage::RenderTarget, "ReferenceSceneColor");
	    },
	    [lighting](PassExecutionContext& context)
	    {
		    for (FrameGraphTextureHandle target : GetLightingTargets(lighting))
		    {
			    context.Resources.ClearRenderTarget(context.Commands, target);
		    }
	    });
}
