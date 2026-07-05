#include "../../PCH.h"
#include "Frame/RayTracing/RayTracingScene.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"

namespace RayTracingSceneFrameGraphContract
{
	constexpr std::string_view kSceneBuildPassName = "RayTracingSceneBuild";
}  // namespace RayTracingSceneFrameGraphContract

RayTracingSceneFrameGraphResources CreateRayTracingSceneFrameGraphResources(FrameGraphBuilder& builder)
{
	return RayTracingSceneFrameGraphResources{
	    .SceneTlas = builder.ReservePersistentAccelerationStructure(FrameGraphAccelerationStructureDesc::Create("SceneTlas"))};
}

void AddRayTracingSceneBuildPasses(FrameGraphBuilder& builder, const RayTracingSceneFrameGraphResources& resources)
{
	builder.AddPass(
	    RayTracingSceneFrameGraphContract::kSceneBuildPassName,
	    EFrameGraphPassFlags::Compute,
	    [resources](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!resources.HasSceneTlas() || !frame.rayTracingScene.HasBoundTlas())
		    {
			    return;
		    }

		    resourceBuilder.Use(
		        resources.SceneTlas,
		        ResourceUsage::AccelerationStructureBuild,
		        "SceneTopLevelAccelerationStructure");
	    },
	    [](PassExecutionContext& context)
	    {
		    if (!context.Frame.rayTracingScene.HasBoundTlas() || context.RuntimeServices.RayTracing == nullptr ||
		        context.RuntimeServices.RayTracing->Scene == nullptr)
		    {
			    return;
		    }

		    context.RuntimeServices.RayTracing->Scene->Build(context.Commands, context.Frame.sceneData, &context.Diagnostics);
	    });
}

void AddRayTracingInfrastructurePasses(FrameGraphBuilder& builder, FrameAssemblyResourceLayout& resources)
{
	resources.Persistent.RayTracing = CreateRayTracingSceneFrameGraphResources(builder);
	resources.Persistent.SceneTlas = resources.Persistent.RayTracing.SceneTlas;
	AddRayTracingSceneBuildPasses(builder, resources.Persistent.RayTracing);
}
