#include "../../PCH.h"
#include "Passes/Scene/RayTracingScenePass.h"

#include "Frame/RenderFrame.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "View/RenderView.h"

static constexpr std::string_view RayTracingSceneBuildPassName = "RayTracingSceneBuild";

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_rayTracingSceneFrameGraphLogger, "Renderer.RayTracingSceneFrameGraph");

void AddRayTracingScenePass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources)
{
	resources.SceneTlas = builder.ReservePersistentAccelerationStructure("SceneTlas");
	const FrameGraphAccelerationStructureHandle sceneTlas = resources.SceneTlas;
	builder.AddPass(
	    RayTracingSceneBuildPassName,
	    EFrameGraphPassKind::Compute,
	    [sceneTlas](PassResourceBuilder& resourceBuilder)
	    {
		    if (!sceneTlas.IsValid())
		    {
			    Diagnostics::Fatal(
			        g_rayTracingSceneFrameGraphLogger,
			        __FILE__,
			        __LINE__,
			        "Ray-tracing scene build received an invalid persistent SceneTlas handle.");
		    }

		    resourceBuilder.Use(sceneTlas, ResourceUsage::AccelerationStructureBuild, "SceneTopLevelAccelerationStructure");
	    },
	    [&rayTracingScene, &frame](PassCommandContext& context)
	    { rayTracingScene.Build(context.Commands, frame.PreparedScene, frame.View.rayTracingPlan, &context.Diagnostics); });
}
