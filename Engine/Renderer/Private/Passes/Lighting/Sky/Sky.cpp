#include "../../../PCH.h"
#include "Passes/Lighting/Sky/Sky.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Sky/SkyPass.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

void AddSkyPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle output,
    FrameGraphTextureHandle sceneDepth,
    FrameGraphTextureHandle sky)
{
	auto& parameters = builder.AllocParameters<SkyPass::Parameters>();
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
	builder.Dispatch<SkyPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
