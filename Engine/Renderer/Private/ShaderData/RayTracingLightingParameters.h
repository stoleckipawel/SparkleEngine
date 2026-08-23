#pragma once

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassInput.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/SkyUniformData.h"
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

	if constexpr (requires { parameterFields->Frame; })
	{
		builder.AddParameterSetup<FrameUniformData>(
		    parameters,
		    [](auto& fields, const FrameUniformData& frame) { fields.Frame = frame; });
	}
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
		    fields.SceneLighting = scene.gpuBindings->Lighting.Uniform;
		    if constexpr (requires { fields.Sky; })
		    {
			    fields.Sky = MakeSkyUniformData(scene.sky);
		    }
		    fields.MaterialTextureTable = scene.materialTextureTable.Binding;
	    });
	builder.AddParameterSetup<RayTracedShadowPassInput>(
	    parameters,
	    [](auto& fields, const RayTracedShadowPassInput& input)
	    { fields.RayTracedShadows = RayTracedShadowPassData::Build(input); });
}
