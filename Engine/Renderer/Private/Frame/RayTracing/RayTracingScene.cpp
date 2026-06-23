#include "../../PCH.h"
#include "Frame/RayTracing/RayTracingScene.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"

namespace RayTracingSceneFrameGraphContract
{
	constexpr std::string_view kLogicalUpdatePassName = "RayTracingPtlasLogicalUpdates";
	constexpr std::string_view kNativeOperationPackPassName = "RayTracingPtlasNativeOperationPack";
	constexpr std::string_view kSceneBuildPassName = "RayTracingSceneBuild";

	constexpr std::uint64_t kReservedPtlasContractBufferSizeInBytes = 1;
	constexpr std::uint32_t kReservedPtlasContractBufferStrideInBytes = 1;
}  // namespace RayTracingSceneFrameGraphContract

RayTracingSceneFrameGraphResources CreateRayTracingSceneFrameGraphResources(FrameGraphBuilder& builder)
{
	return RayTracingSceneFrameGraphResources{
	    .SceneTlas =
	        builder.ReservePersistentAccelerationStructure(FrameGraphAccelerationStructureDesc::Create("SceneTlas")),
	    .PtlasLogicalUpdateRecords =
	        builder.ReservePersistentBuffer(
	            FrameGraphBufferDesc::Create(
	                "RayTracingPtlasLogicalUpdateRecords",
	                RayTracingSceneFrameGraphContract::kReservedPtlasContractBufferSizeInBytes,
	                RayTracingSceneFrameGraphContract::kReservedPtlasContractBufferStrideInBytes)),
	    .PtlasNativeOperationData =
	        builder.ReservePersistentBuffer(
	            FrameGraphBufferDesc::Create(
	                "RayTracingPtlasNativeOperationData",
	                RayTracingSceneFrameGraphContract::kReservedPtlasContractBufferSizeInBytes,
	                RayTracingSceneFrameGraphContract::kReservedPtlasContractBufferStrideInBytes)),
	    .PtlasScratch =
	        builder.ReservePersistentBuffer(
	            FrameGraphBufferDesc::Create(
	                "RayTracingPtlasScratch",
	                RayTracingSceneFrameGraphContract::kReservedPtlasContractBufferSizeInBytes,
	                RayTracingSceneFrameGraphContract::kReservedPtlasContractBufferStrideInBytes))};
}

void AddRayTracingSceneBuildPasses(FrameGraphBuilder& builder, const RayTracingSceneFrameGraphResources& resources)
{
	builder.AddPass(
	    RayTracingSceneFrameGraphContract::kLogicalUpdatePassName,
	    EFrameGraphPassFlags::Compute,
	    [resources](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!resources.HasPartitionedTlasResources() || !frame.rayTracingScene.HasPartitionedTlasOperationResources())
		    {
			    return;
		    }

		    resourceBuilder.Use(
		        resources.PtlasLogicalUpdateRecords,
		        ResourceUsage::UnorderedAccess,
		        "LogicalUpdateRecords");
	    },
	    [](PassExecutionContext& context)
	    {
		    if (context.RuntimeServices.RayTracing == nullptr || context.RuntimeServices.RayTracing->Scene == nullptr)
		    {
			    return;
		    }

		    context.RuntimeServices.RayTracing->Scene->BuildPartitionedTlasLogicalUpdateResources(
		        context.Commands,
		        context.Frame.sceneData,
		        &context.Diagnostics);
	    });

	builder.AddPass(
	    RayTracingSceneFrameGraphContract::kNativeOperationPackPassName,
	    EFrameGraphPassFlags::Compute,
	    [resources](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!resources.HasPartitionedTlasResources() || !frame.rayTracingScene.HasPartitionedTlasOperationResources())
		    {
			    return;
		    }

		    resourceBuilder.Read(
		        resources.PtlasLogicalUpdateRecords,
		        ResourceUsage::ShaderRead,
		        "LogicalUpdateRecords");
		    resourceBuilder.Use(
		        resources.PtlasNativeOperationData,
		        ResourceUsage::UnorderedAccess,
		        "NativeOperationCountAndRecords");
	    },
	    [](PassExecutionContext& context)
	    {
		    if (context.RuntimeServices.RayTracing == nullptr || context.RuntimeServices.RayTracing->Scene == nullptr)
		    {
			    return;
		    }

		    context.RuntimeServices.RayTracing->Scene->PackPartitionedTlasNativeOperations(
		        context.Commands,
		        context.Frame.sceneData,
		        &context.Diagnostics);
	    });

	builder.AddPass(
	    RayTracingSceneFrameGraphContract::kSceneBuildPassName,
	    EFrameGraphPassFlags::Compute,
	    [resources](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!resources.HasSceneTlas() || !frame.rayTracingScene.HasBoundTlas())
		    {
			    return;
		    }

		    if (resources.HasPartitionedTlasResources() && frame.rayTracingScene.HasPartitionedTlasOperationResources())
		    {
			    resourceBuilder.Read(
			        resources.PtlasNativeOperationData,
			        ResourceUsage::ShaderRead,
			        "NativeOperationCountAndRecords");
			    resourceBuilder.Use(
			        resources.PtlasScratch,
			        ResourceUsage::UnorderedAccess,
			        "PartitionedTlasScratch");
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
