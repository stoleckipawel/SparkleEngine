#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerResources.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerShader.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerUniformData.h"
#include "RayTracing/Effects/RayTracingExecutionFrontend.h"
#include "RayTracing/RayTracingMaterialPipelineShaders.h"
#include "RayTracing/RayTracingPipelineComposition.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "ShaderData/SkyUniformData.h"
#include "View/RenderView.h"

#include <vector>

template <typename TShader> static auto& BuildReferencePathTracerParameters(
    FrameGraphBuilder& builder,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData)
{
	auto& parameters = builder.AllocParameters<TShader>();
	parameters->WorkingMean = builder.CreateUAV(graphResources.WorkingMean);
	parameters->WorkingM2 = builder.CreateUAV(graphResources.WorkingM2);
	parameters->CommittedMean = builder.CreateSRV(graphResources.CommittedMean);
	parameters->CommittedM2 = builder.CreateSRV(graphResources.CommittedM2);
	parameters->SceneTlas = builder.CreateAccelerationStructureBinding(resources.SceneTlas);
	parameters->SkyTexture = builder.CreateSRV(resources.ImportedScene.Sky);
	parameters->SamplerLinearWrapClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::None,
	    .Address =
	        RhiSamplerAddressModes{.U = RhiSamplerAddressMode::Wrap, .V = RhiSamplerAddressMode::Clamp, .W = RhiSamplerAddressMode::Clamp}};
	parameters->DirectionalLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.PointLights);
	parameters->SpotLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.SpotLights);
	parameters->RectLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.RectLights);
	parameters->RayTracingHitVertices = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Vertices);
	parameters->SkinInfluences = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.SkinInfluences);
	parameters->MorphTargetDeltas = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.MorphTargetDeltas);
	parameters->RayTracingHitIndices = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Indices);
	parameters->RayTracingHitInstances = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Instances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Materials);
	parameters->MeshInstances = builder.CreateSRV(resources.ImportedScene.Scene.Geometry.MeshInstances);
	parameters->JointMatrices = builder.CreateSRV(resources.ImportedScene.Scene.Geometry.JointMatrices);
	parameters->MorphWeights = builder.CreateSRV(resources.ImportedScene.Scene.Geometry.MorphWeights);
	builder.AddPassParameterSetup(
	    parameters,
	    [uniformData = &uniformData](auto& fields) { fields.ReferencePathTracerConstants = *uniformData; });
	builder.AddParameterSetup<RenderView>(parameters, [](auto& fields, const RenderView& view) { fields.ViewCamera = view.cameraUniform; });
	builder.AddParameterSetup<PreparedRenderScene>(
	    parameters,
	    [](auto& fields, const PreparedRenderScene& scene)
	    {
		    fields.Sky = MakeSkyUniformData(scene.sky);
		    fields.SceneLighting = scene.gpuBindings->Lighting.Uniform;
		    fields.RayTracingHitConstants = RayTracingHitUniformData{
		        .RayTracingHitInstanceCount = scene.gpuBindings->RayTracing.InstanceCount,
		        .RayTracingHitMaterialCount = scene.gpuBindings->RayTracing.MaterialCount};
		    fields.MaterialTextureTable = scene.materialTextureTable.Binding;
	    });
	return parameters;
}

static void AddReferencePathTracerInlinePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData,
    std::uint32_t workRowsPerDispatch)
{
	auto& parameters = BuildReferencePathTracerParameters<ReferencePathTracerInlineCS>(builder, resources, graphResources, uniformData);
	builder.Dispatch<ReferencePathTracerInlineCS>(
	    "ReferencePathTracer.SurfaceTransportReference",
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(extent.Width, 8u), MathUtils::DivideRoundUp(workRowsPerDispatch, 8u), 1u});
}

static void AddReferencePathTracerPipelinePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData,
    std::uint32_t workRowsPerDispatch,
    RayTracingShaderTablePlan& shaderTablePlan)
{
	auto& parameters = BuildReferencePathTracerParameters<ReferencePathTracerRGS>(builder, resources, graphResources, uniformData);
	const RayTracingPipelineComposition composition = RayTracingPipelineComposition::Create<ReferencePathTracerRGS>(
	    std::vector{RayTracingPipelineComposition::Shader<RayTracingMaterialMiss>()},
	    std::vector{
	        RayTracingHitGroupComposition::Triangles<RayTracingMaterialClosestHit>("RayTracingMaterialOpaqueHitGroup"),
	        RayTracingHitGroupComposition::Triangles<RayTracingMaterialClosestHit, RayTracingMaterialAnyHit>(
	            "RayTracingMaterialAlphaTestedHitGroup")});
	builder.TraceRays<ReferencePathTracerRGS>(
	    "ReferencePathTracer.SurfaceTransportReference",
	    composition,
	    shaderTablePlan,
	    parameters,
	    RayTracingDispatchDimensions{.Width = extent.Width, .Height = workRowsPerDispatch, .Depth = 1u});
}

static void AddReferencePathTracerDisplayPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData)
{
	auto& display = builder.AllocParameters<ReferencePathTracerDisplayCS>();
	display->WorkingMean = builder.CreateSRV(graphResources.WorkingMean);
	display->WorkingM2 = builder.CreateSRV(graphResources.WorkingM2);
	display->CommittedMean = builder.CreateUAV(graphResources.CommittedMean);
	display->CommittedM2 = builder.CreateUAV(graphResources.CommittedM2);
	display->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	builder.AddPassParameterSetup(
	    display,
	    [uniformData = &uniformData](auto& fields) { fields.ReferencePathTracerConstants = *uniformData; });
	builder.Dispatch<ReferencePathTracerDisplayCS>(
	    "ReferencePathTracer.CommittedDisplay",
	    display,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(extent.Width, 8u), MathUtils::DivideRoundUp(extent.Height, 8u), 1u});
}

void AddReferencePathTracerGpuPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData,
    std::uint32_t workRowsPerDispatch,
    RayTracingExecutionFrontend executionFrontend,
    RayTracingShaderTablePlan& shaderTablePlan)
{
	if (executionFrontend == RayTracingExecutionFrontend::Inline)
	{
		AddReferencePathTracerInlinePass(builder, extent, resources, graphResources, uniformData, workRowsPerDispatch);
	}
	else if (executionFrontend == RayTracingExecutionFrontend::Pipeline)
	{
		AddReferencePathTracerPipelinePass(builder, extent, resources, graphResources, uniformData, workRowsPerDispatch, shaderTablePlan);
	}
	AddReferencePathTracerDisplayPass(builder, extent, resources, graphResources, uniformData);
}
