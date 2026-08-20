#include "../../PCH.h"
#include "Frame/RayTracing/RayTracingScene.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"

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
};

static const auto g_rayTracingSceneFrameGraphLogger = Logging::GetOrCreateLogger("Renderer.RayTracingSceneFrameGraph");

FrameGraphAccelerationStructureHandle CreateRayTracingSceneFrameGraphResource(FrameGraphBuilder& builder)
{
	return builder.ReservePersistentAccelerationStructure(FrameGraphAccelerationStructureDesc::Create("SceneTlas"));
}

void AddRayTracingSceneBuildPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto input = std::make_shared<RayTracingSceneBuildPassInput>();
	builder.AddPreparedSceneSetup([input](const PreparedRenderScene& preparedScene) { input->PreparedScene = std::cref(preparedScene); });
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
		    if (!input->PreparedScene.has_value() || !rayTracingScene.HasValidTlas())
		    {
			    Diagnostics::Fatal(
			        g_rayTracingSceneFrameGraphLogger,
			        __FILE__,
			        __LINE__,
			        "Ray-tracing scene build did not receive a bound SceneTlas and active scene producer.");
		    }

		    rayTracingScene.Build(context.Commands, input->PreparedScene->get(), &context.Diagnostics);
	    });
}

void AddRaytracingScenePasses(FrameGraphBuilder& builder, RenderRayTracingScene& rayTracingScene, FrameAssemblyResourceLayout& resources)
{
	resources.SceneTlas = CreateRayTracingSceneFrameGraphResource(builder);
	AddRayTracingSceneBuildPasses(builder, rayTracingScene, resources.SceneTlas);
}
