#include "../../PCH.h"
#include "Frame/Lighting/Sky.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SkyPass.h"
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
	auto* parameterFields = parameters.operator->();
	parameters->SceneColor = builder.CreateUAV(output);
	parameters->SceneDepth = builder.CreateSRV(sceneDepth);
	parameters->SkyTexture = builder.CreateSRV(sky);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	builder.AddRenderViewSetup(
	    [parameterFields](const RenderView& view)
	    {
		    parameterFields->View = view.uniform;
		    parameterFields->ViewCamera = view.cameraUniform;
		    parameterFields->ViewTemporal = view.temporalUniform;
	    });
	builder.AddPreparedSceneSetup(
	    [parameterFields](const PreparedRenderScene& scene) { parameterFields->Sky = MakeSkyUniformData(scene.sky); });
	builder.Dispatch<SkyPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
