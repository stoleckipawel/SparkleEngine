#include "../../PCH.h"
#include "Passes/RayTracing/RayTracingScene.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "View/RenderView.h"

#include <functional>
#include <memory>
#include <optional>

namespace RayTracingSceneFrameGraphContract
{
	constexpr std::string_view kSceneBuildPassName = "RayTracingSceneBuild";
}

struct RayTracingSceneBuildPassInput final
{
	// Populated during graph setup and consumed synchronously while recording this frame.
	std::optional<std::reference_wrapper<const PreparedRenderScene>> PreparedScene;
	std::optional<std::reference_wrapper<const RayTracingPtlasPartitionPlan>> ViewPlan;
};

static const auto g_rayTracingSceneFrameGraphLogger = Logging::GetOrCreateLogger("Renderer.RayTracingSceneFrameGraph");

FrameGraphAccelerationStructureHandle CreateRayTracingSceneFrameGraphResource(FrameGraphBuilder& builder)
{
	return builder.ReservePersistentAccelerationStructure("SceneTlas");
}

void AddRayTracingSceneBuildPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto input = std::make_shared<RayTracingSceneBuildPassInput>();
	builder.AddParameterSetup<PreparedRenderScene>(
	    [input](const PreparedRenderScene& preparedScene) { input->PreparedScene = std::cref(preparedScene); });
	builder.AddParameterSetup<RenderView>(
	    [input](const RenderView& view) { input->ViewPlan = std::cref(view.rayTracingPlan); });
	builder.AddPass(
	    RayTracingSceneFrameGraphContract::kSceneBuildPassName,
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
	    [&rayTracingScene, input](PassCommandContext& context)
	    {
		    if (!input->PreparedScene.has_value() || !input->ViewPlan.has_value())
		    {
			    Diagnostics::Fatal(
			        g_rayTracingSceneFrameGraphLogger,
			        __FILE__,
			        __LINE__,
			        "Ray-tracing scene build did not receive the current prepared scene and view plan.");
		    }

		    rayTracingScene.Build(context.Commands, input->PreparedScene->get(), input->ViewPlan->get(), &context.Diagnostics);
	    });
}

void AddRaytracingScenePasses(FrameGraphBuilder& builder, RenderRayTracingScene& rayTracingScene, RenderFrameGraphResources& resources)
{
	resources.SceneTlas = CreateRayTracingSceneFrameGraphResource(builder);
	AddRayTracingSceneBuildPasses(builder, rayTracingScene, resources.SceneTlas);
}
