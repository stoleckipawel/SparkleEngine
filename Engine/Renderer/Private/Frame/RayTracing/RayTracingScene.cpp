#include "../../PCH.h"
#include "Frame/RayTracing/RayTracingScene.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Scene/RenderScene.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"

namespace RayTracingSceneFrameGraphContract
{
	constexpr std::string_view kSceneBuildPassName = "RayTracingSceneBuild";
} // namespace RayTracingSceneFrameGraphContract

static const auto g_rayTracingSceneFrameGraphLogger = Logging::GetOrCreateLogger("Renderer.RayTracingSceneFrameGraph");

FrameGraphAccelerationStructureHandle CreateRayTracingSceneFrameGraphResource(FrameGraphBuilder& builder)
{
	return builder.ReservePersistentAccelerationStructure(FrameGraphAccelerationStructureDesc::Create("SceneTlas"));
}

void AddRayTracingSceneBuildPasses(FrameGraphBuilder& builder, FrameGraphAccelerationStructureHandle sceneTlas)
{
	builder.AddPass(
	    RayTracingSceneFrameGraphContract::kSceneBuildPassName,
	    EFrameGraphPassKind::Compute,
	    [sceneTlas](PassResourceBuilder& resourceBuilder, const FrameContext&)
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
	    [](PassExecutionContext& context)
	    {
		    if (context.Runtime.RayTracing == nullptr || context.Runtime.RayTracing->Scene == nullptr
		        || !context.Runtime.RayTracing->Scene->HasValidRayTracingTlas())
		    {
			    Diagnostics::Fatal(
			        g_rayTracingSceneFrameGraphLogger,
			        __FILE__,
			        __LINE__,
			        "Ray-tracing scene build did not receive a bound SceneTlas and active scene producer.");
		    }

		    context.Runtime.RayTracing->Scene->BuildRayTracingScene(context.Commands, context.Frame.preparedScene, &context.Diagnostics);
	    });
}

void AddRaytracingScenePasses(FrameGraphBuilder& builder, FrameAssemblyResourceLayout& resources)
{
	resources.SceneTlas = CreateRayTracingSceneFrameGraphResource(builder);
	AddRayTracingSceneBuildPasses(builder, resources.SceneTlas);
}
