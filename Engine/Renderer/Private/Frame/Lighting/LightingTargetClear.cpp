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
		    lighting.IndirectSubsurface};
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
	    },
	    [lighting](PassExecutionContext& context)
	    {
		    for (FrameGraphTextureHandle target : GetLightingTargets(lighting))
		    {
			    context.Resources.ClearRenderTarget(context.Commands, target);
		    }
	    });
}
