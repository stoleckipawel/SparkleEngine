#include "../../../PCH.h"
#include "Passes/Lighting/Sky/Sky.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Sky/SkyShader.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

void AddSkyPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle output,
    FrameGraphTextureHandle sceneDepth,
    FrameGraphTextureHandle sky)
{
	auto& parameters = builder.AllocParameters<SkyCS>();
	parameters->SceneColor = builder.CreateUAV(output);
	parameters->SceneDepth = builder.CreateSRV(sceneDepth);
	parameters->SkyTexture = builder.CreateSRV(sky);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
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
	    [](auto& fields, const PreparedRenderScene& scene) { fields.Sky = MakeSkyUniformData(scene.sky); });
	builder.Dispatch<SkyCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
