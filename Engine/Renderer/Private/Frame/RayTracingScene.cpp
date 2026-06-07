#include "../PCH.h"
#include "Frame/RayTracingScene.h"

#include "Frame/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayTracing/RenderRayTracingScene.h"

namespace
{
	constexpr std::string_view kPassName = "RayTracingSceneBuild";
}

void AddRayTracingSceneBuildPass(FrameGraphBuilder& builder, FrameGraphAccelerationStructureHandle sceneTlas)
{
	builder.AddPass(
	    kPassName,
	    EFrameGraphPassFlags::Compute,
	    [sceneTlas](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!sceneTlas.IsValid() || !frame.rayTracingScene.HasBoundTlas())
		    {
			    return;
		    }

		    resourceBuilder.Write(sceneTlas, ResourceUsage::AccelerationStructureBuild);
	    },
	    [](PassExecutionContext& context)
	    {
		    if (!context.Frame.rayTracingScene.HasBoundTlas() || context.RuntimeServices.RayTracing == nullptr ||
		        context.RuntimeServices.RayTracing->Scene == nullptr)
		    {
			    return;
		    }

		    context.RuntimeServices.RayTracing->Scene->Build(context.Commands, context.Frame.sceneData);
	    });
}
