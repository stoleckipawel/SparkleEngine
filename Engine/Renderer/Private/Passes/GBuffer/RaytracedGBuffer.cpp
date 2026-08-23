#include "PCH.h"
#include "Passes/GBuffer/RaytracedGBuffer.h"

#include "Core/Public/Math/MathUtils.h"
#include "Passes/GBuffer/RaytracedGBufferTargetClear.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RaytracedGBufferShader.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

void AddRaytracedGBufferMeshPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const RenderFrameGraphImportedSceneResources& externalResources)
{
	AddRaytracedGBufferTargetClearPass(builder, targets);
	auto& parameters = builder.AllocParameters<RaytracedGBufferCS>();
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
	parameters->MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};
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
		    fields.RaytracedGBufferConstants = RaytracedGBufferUniformData{
		        .RayTracingHitInstanceCount = rayTracing.InstanceCount,
		        .RayTracingHitMaterialCount = rayTracing.MaterialCount};
	    });
	builder.Dispatch<RaytracedGBufferCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
