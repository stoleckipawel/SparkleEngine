#include "../../../PCH.h"
#include "Frame/Lighting/Shadows/DirectShadowSignal.h"

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectShadowSignalPass.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassInput.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const DirectShadowSignalResources& shadowSignals,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& descriptorParameters = builder.AllocParameters<DirectShadowSignalPass::Parameters>();
	auto* descriptorFields = descriptorParameters.operator->();
	descriptorParameters->ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
	descriptorParameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Current);
	descriptorParameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Current);
	descriptorParameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
	descriptorParameters->DirectionalLights = builder.CreateSRV(externalResources.Scene.Lighting.DirectionalLights);
	descriptorParameters->PointLights = builder.CreateSRV(externalResources.Scene.Lighting.PointLights);
	descriptorParameters->SpotLights = builder.CreateSRV(externalResources.Scene.Lighting.SpotLights);
	descriptorParameters->RectLights = builder.CreateSRV(externalResources.Scene.Lighting.RectLights);
	descriptorParameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	descriptorParameters->RayTracingHitVertices = builder.CreateSRV(externalResources.Scene.RayTracing.Vertices);
	descriptorParameters->RayTracingHitIndices = builder.CreateSRV(externalResources.Scene.RayTracing.Indices);
	descriptorParameters->RayTracingHitInstances = builder.CreateSRV(externalResources.Scene.RayTracing.Instances);
	descriptorParameters->RayTracingHitMaterials = builder.CreateSRV(externalResources.Scene.RayTracing.Materials);
	descriptorParameters->SceneTlas = builder.Read(sceneTlas);
	descriptorParameters->MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};
	builder.AddFrameUniformSetup([descriptorFields](const FrameUniformData& frame) { descriptorFields->Frame = frame; });
	builder.AddRenderViewSetup(
	    [descriptorFields](const RenderView& view)
	    {
		    descriptorFields->View = view.uniform;
		    descriptorFields->ViewCamera = view.cameraUniform;
		    descriptorFields->ViewTemporal = view.temporalUniform;
	    });
	builder.AddPreparedSceneSetup(
	    [descriptorFields](const PreparedRenderScene& scene)
	    {
		    descriptorFields->SceneLighting = scene.gpuBindings->Lighting.Uniform;
		    descriptorFields->MaterialTextureTable = scene.materialTextureTable.Binding;
	    });
	builder.AddRayTracedShadowSetup(
	    [descriptorFields](const PreparedRenderScene& scene, const RayTracedShadowPassInput& shadowInput)
	    {
		    const auto& rayTracing = scene.gpuBindings->RayTracing;
		    descriptorFields->RayTracedShadows = RayTracedShadowPassData::Build(
		        shadowInput,
		        rayTracing.InstanceCount > 0u,
		        rayTracing.InstanceCount,
		        rayTracing.MaterialCount);
	    });
	builder.Dispatch<DirectShadowSignalPass>(descriptorParameters, sceneExtent.Width, sceneExtent.Height);
}
