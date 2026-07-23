#include "PCH.h"
#include "Frame/GBuffer/RaytracedGBufferTargetClear.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/ResourceUsage.h"

#include <array>

class RaytracedGBufferTargetClearOperations final
{
  public:
	static auto GetRaytracedGBufferTargets(const GBufferRenderTargets& targets) noexcept
	{
		return std::array{
		    targets.BaseColor,
		    targets.Normal,
		    targets.Material,
		    targets.Emissive,
		    targets.Subsurface,
		    targets.DeviceZ,
		    targets.MotionVector};
	}
};

void AddRaytracedGBufferTargetClearPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets)
{
	builder.Execute(
	    "RaytracedGBufferTargetClear",
	    EFrameGraphPassKind::Raster,
	    [targets](PassResourceBuilder& resourceBuilder)
	    {
		    for (const FrameGraphTextureHandle target : RaytracedGBufferTargetClearOperations::GetRaytracedGBufferTargets(targets))
		    {
			    resourceBuilder.Write(target, ResourceUsage::RenderTarget, "RaytracedGBufferTarget");
		    }
	    },
	    [targets](PassExecutionContext& context)
	    {
		    for (const FrameGraphTextureHandle target : RaytracedGBufferTargetClearOperations::GetRaytracedGBufferTargets(targets))
		    {
			    context.Resources.ClearRenderTarget(context.Commands, target);
		    }
	    });
}
