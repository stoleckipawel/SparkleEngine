#include "../../PCH.h"
#include "Frame/Lighting/Sky.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SkyPass.h"

void AddSkyPass(FrameGraphBuilder& builder, FrameGraphTextureHandle output, FrameGraphTextureHandle sceneDepth, FrameGraphTextureHandle sky)
{
	auto& parameters = builder.AllocParameters<SkyPass::Parameters>();
	parameters->SceneColor = builder.CreateUAV(output);
	parameters->SceneDepth = builder.CreateSRV(sceneDepth);
	parameters->SkyTexture = builder.CreateSRV(sky);
	builder.Dispatch<SkyPass>(parameters);
}
