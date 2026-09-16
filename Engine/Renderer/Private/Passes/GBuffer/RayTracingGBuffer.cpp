#include "PCH.h"

#include "Passes/GBuffer/RayTracingGBuffer.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RayTracingGBufferShaders.h"
#include "RayTracing/Effects/RayTracingExecutionFrontend.h"
#include "RayTracing/RayTracingMaterialPipelineShaders.h"
#include "RayTracing/RayTracingPipelineComposition.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

#include <vector>

namespace RayTracingGBufferPasses
{
	template <typename TShader> auto& BuildParameters(
	    FrameGraphBuilder& builder,
	    const GBufferRenderTargets& targets,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const RenderFrameGraphImportedSceneResources& externalResources)
	{
		auto& parameters = builder.AllocParameters<TShader>();
		parameters->GBufferBaseColor = builder.CreateUAV(targets.BaseColor);
		parameters->GBufferNormal = builder.CreateUAV(targets.Normal);
		parameters->GBufferMaterial = builder.CreateUAV(targets.Material);
		parameters->GBufferEmissive = builder.CreateUAV(targets.Emissive);
		parameters->GBufferSubsurface = builder.CreateUAV(targets.Subsurface);
		parameters->GBufferDeviceZ = builder.CreateUAV(targets.DeviceZ);
		parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
		parameters->SceneTlas = builder.CreateAccelerationStructureBinding(sceneTlas);
		parameters->RayTracingHitVertices = builder.CreateSRV(externalResources.Scene.RayTracing.Vertices);
		parameters->MorphTargetDeltas = builder.CreateSRV(externalResources.Scene.RayTracing.MorphTargetDeltas);
		parameters->SkinInfluences = builder.CreateSRV(externalResources.Scene.RayTracing.SkinInfluences);
		parameters->RayTracingHitIndices = builder.CreateSRV(externalResources.Scene.RayTracing.Indices);
		parameters->RayTracingHitInstances = builder.CreateSRV(externalResources.Scene.RayTracing.Instances);
		parameters->RayTracingHitMaterials = builder.CreateSRV(externalResources.Scene.RayTracing.Materials);
		parameters->MeshInstances = builder.CreateSRV(externalResources.Scene.Geometry.MeshInstances);
		parameters->JointMatrices = builder.CreateSRV(externalResources.Scene.Geometry.JointMatrices);
		parameters->PreviousJointMatrices = builder.CreateSRV(externalResources.Scene.Geometry.PreviousJointMatrices);
		parameters->MorphWeights = builder.CreateSRV(externalResources.Scene.Geometry.MorphWeights);
		parameters->PreviousMorphWeights = builder.CreateSRV(externalResources.Scene.Geometry.PreviousMorphWeights);
		builder.AddParameterSetup<RenderView>(
		    parameters,
		    [](auto& fields, const RenderView& view)
		    {
			    fields.View = view.uniform;
			    fields.ViewCamera = view.cameraUniform;
			    fields.ViewTemporal = view.temporalUniform;
		    });
		builder.AddParameterSetup<PreparedRenderScene>(
		    parameters,
		    [](auto& fields, const PreparedRenderScene& scene)
		    {
			    const auto& rayTracing = scene.gpuBindings->RayTracing;
			    fields.MaterialTextureTable = scene.materialTextureTable.Binding;
			    fields.RayTracingHitConstants = RayTracingHitUniformData{
			        .RayTracingHitInstanceCount = rayTracing.InstanceCount,
			        .RayTracingHitMaterialCount = rayTracing.MaterialCount};
		    });
		return parameters;
	}

	void AddInline(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent,
	    const GBufferRenderTargets& targets,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const RenderFrameGraphImportedSceneResources& externalResources)
	{
		auto& parameters = BuildParameters<RayTracingGBufferInlineCS>(builder, targets, sceneTlas, externalResources);
		builder.Dispatch<RayTracingGBufferInlineCS>(
		    "RayTracingGBuffer.Inline",
		    parameters,
		    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
	}

	void AddPipeline(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent,
	    const GBufferRenderTargets& targets,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const RenderFrameGraphImportedSceneResources& externalResources,
	    RayTracingShaderTablePlan& shaderTablePlan)
	{
		auto& parameters = BuildParameters<RayTracingGBufferRGS>(builder, targets, sceneTlas, externalResources);
		const RayTracingPipelineComposition composition = RayTracingPipelineComposition::Create<RayTracingGBufferRGS>(
		    std::vector{RayTracingPipelineComposition::Shader<RayTracingMaterialMiss>()},
		    std::vector{
		        RayTracingHitGroupComposition::Triangles<RayTracingMaterialClosestHit>("RayTracingMaterialOpaqueHitGroup"),
		        RayTracingHitGroupComposition::Triangles<RayTracingMaterialClosestHit, RayTracingMaterialAnyHit>(
		            "RayTracingMaterialAlphaTestedHitGroup")});
		builder.TraceRays<RayTracingGBufferRGS>(
		    "RayTracingGBuffer.Pipeline",
		    composition,
		    shaderTablePlan,
		    parameters,
		    RayTracingDispatchDimensions{.Width = sceneExtent.Width, .Height = sceneExtent.Height, .Depth = 1u});
	}
}

void AddRayTracingGBufferMeshPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const RenderFrameGraphImportedSceneResources& externalResources,
    RayTracingShaderTablePlan& shaderTablePlan,
    const RayTracingCapabilityReport& capabilities)
{
	switch (ResolveRayTracingExecutionFrontend(capabilities))
	{
		case RayTracingExecutionFrontend::Inline:
			RayTracingGBufferPasses::AddInline(builder, sceneExtent, targets, sceneTlas, externalResources);
			return;
		case RayTracingExecutionFrontend::Pipeline:
			RayTracingGBufferPasses::AddPipeline(builder, sceneExtent, targets, sceneTlas, externalResources, shaderTablePlan);
			return;
		case RayTracingExecutionFrontend::None:
		default:
			throw Diagnostics::Error("Ray-traced GBuffer graph construction received no ready execution frontend.");
	}
}
