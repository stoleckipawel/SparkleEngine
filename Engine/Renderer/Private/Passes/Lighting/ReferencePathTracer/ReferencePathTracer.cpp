#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerShader.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "ShaderData/SkyUniformData.h"
#include "View/RenderView.h"

void ReferencePathTracer::AddPass(FrameGraphBuilder& builder, RenderViewportExtent extent, const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<ReferencePathTracerCS>();
	parameters->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->SceneTlas = builder.CreateAccelerationStructureBinding(resources.SceneTlas);
	parameters->SkyTexture = builder.CreateSRV(resources.ImportedScene.Sky);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	parameters->RayTracingHitVertices = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Vertices);
	parameters->RayTracingHitIndices = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Indices);
	parameters->RayTracingHitInstances = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Instances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Materials);
	parameters->MeshInstances = builder.CreateSRV(resources.ImportedScene.Scene.Geometry.MeshInstances);
	builder.AddParameterSetup<RenderView>(parameters, [](auto& fields, const RenderView& view) { fields.ViewCamera = view.cameraUniform; });
	builder.AddParameterSetup<PreparedRenderScene>(
	    parameters,
	    [](auto& fields, const PreparedRenderScene& scene)
	    {
		    fields.Sky = MakeSkyUniformData(scene.sky);
		    fields.ReferencePathTracerConstants = ReferencePathTracerUniformData{};
		    fields.RayTracingHitConstants = RayTracingHitUniformData{
		        .RayTracingHitInstanceCount = scene.gpuBindings->RayTracing.InstanceCount,
		        .RayTracingHitMaterialCount = scene.gpuBindings->RayTracing.MaterialCount};
	    });
	builder.Dispatch<ReferencePathTracerCS>(
	    "ReferencePathTracer.FinitePathDiagnostic.D2",
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(extent.Width, 8u), MathUtils::DivideRoundUp(extent.Height, 8u), 1u});
}

ViewportRenderProgress ReferencePathTracer::Update(const RenderView& view) const noexcept
{
	if (view.viewMode != RenderViewMode::ReferencePathTracer)
	{
		return {};
	}

	return ViewportRenderProgress{
	    .ViewMode = RenderViewMode::ReferencePathTracer,
	    .State = ViewportRenderProgressState::Unavailable,
	    .CompletedWork = 0,
	    .TargetWork = 0};
}
