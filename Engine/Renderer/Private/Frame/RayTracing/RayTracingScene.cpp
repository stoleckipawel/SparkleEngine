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

FrameGraphAccelerationStructureHandle CreateRayTracingSceneFrameGraphResource(FrameGraphBuilder& builder)
{
	return builder.ReservePersistentAccelerationStructure(FrameGraphAccelerationStructureDesc::Create("SceneTlas"));
}

void AddRayTracingSceneBuildPasses(FrameGraphBuilder& builder, FrameGraphAccelerationStructureHandle sceneTlas)
{
	builder.AddPass(
	    RayTracingSceneFrameGraphContract::kSceneBuildPassName,
	    EFrameGraphPassKind::Compute,
	    [sceneTlas](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!sceneTlas.IsValid() || !frame.rayTracingScene.HasBoundTlas())
		    {
			    return;
		    }

		    resourceBuilder.Use(
		        sceneTlas,
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

void AddRaytracingScenePasses(FrameGraphBuilder& builder, FrameAssemblyResourceLayout& resources)
{
	resources.SceneTlas = CreateRayTracingSceneFrameGraphResource(builder);
	AddRayTracingSceneBuildPasses(builder, resources.SceneTlas);
}
