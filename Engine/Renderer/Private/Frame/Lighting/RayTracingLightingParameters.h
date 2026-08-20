#pragma once

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassInput.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

template <typename TParameterInstance>
void RegisterRayTracingLightingParameterSetups(FrameGraphBuilder& builder, TParameterInstance& parameters)
{
	auto* parameterFields = parameters.operator->();
	if constexpr (requires { parameterFields->SamplerLinearClamp; })
	{
		parameterFields->SamplerLinearClamp = RhiSamplerDesc{
		    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
		    .MipFilter = RhiSamplerMipFilter::Linear,
		    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	}
	parameterFields->MaterialTextureSampler = RhiSamplerDesc{
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
		    parameterFields->SceneLighting = scene.gpuBindings->Lighting.Uniform;
		    if constexpr (requires { parameterFields->Sky; })
		    {
			    parameterFields->Sky = MakeSkyUniformData(scene.sky);
		    }
		    parameterFields->MaterialTextureTable = scene.materialTextureTable.Binding;
	    });
	builder.AddRayTracedShadowSetup(
	    [parameterFields](const PreparedRenderScene& scene, const RayTracedShadowPassInput& shadowInput)
	    {
		    const auto& rayTracing = scene.gpuBindings->RayTracing;
		    parameterFields->RayTracedShadows = RayTracedShadowPassData::Build(
		        shadowInput,
		        rayTracing.InstanceCount > 0u,
		        rayTracing.InstanceCount,
		        rayTracing.MaterialCount);
	    });
}
