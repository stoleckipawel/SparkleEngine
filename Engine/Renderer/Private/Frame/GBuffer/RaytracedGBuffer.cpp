#include "PCH.h"
#include "Frame/GBuffer/RaytracedGBuffer.h"

#include "Frame/GBuffer/RaytracedGBufferTargetClear.h"
#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RaytracedGBufferPass.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

void AddRaytracedGBufferPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const FrameAssemblyExternalResources& externalResources)
{
	AddRaytracedGBufferTargetClearPass(builder, targets);
	auto& parameters = builder.AllocParameters<RaytracedGBufferPass::Parameters>();
	auto* parameterFields = parameters.operator->();
	parameters->GBufferBaseColor = builder.CreateUAV(targets.BaseColor);
	parameters->GBufferNormal = builder.CreateUAV(targets.Normal);
	parameters->GBufferMaterial = builder.CreateUAV(targets.Material);
	parameters->GBufferEmissive = builder.CreateUAV(targets.Emissive);
	parameters->GBufferSubsurface = builder.CreateUAV(targets.Subsurface);
	parameters->GBufferDeviceZ = builder.CreateUAV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
	parameters->SceneTlas = builder.Read(sceneTlas);
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
	builder.AddFrameUniformSetup([parameterFields](const FrameUniformData& frame) { parameterFields->Frame = frame; });
	builder.AddRenderViewSetup(
	    [parameterFields](const RenderView& view)
	    {
		    parameterFields->View = view.uniform;
		    parameterFields->ViewCamera = view.cameraUniform;
		    parameterFields->ViewTemporal = view.temporalUniform;
	    });
	builder.AddPreparedSceneSetup(
	    [parameterFields](const PreparedRenderScene& scene)
	    {
		    const auto& rayTracing = scene.gpuBindings->RayTracing;
		    parameterFields->MaterialTextureTable = scene.materialTextureTable.Binding;
		    parameterFields->RaytracedGBufferConstants = RaytracedGBufferUniformData{
		        .RayTracingHitInstanceCount = rayTracing.InstanceCount,
		        .RayTracingHitMaterialCount = rayTracing.MaterialCount};
	    });
	builder.Dispatch<RaytracedGBufferPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
