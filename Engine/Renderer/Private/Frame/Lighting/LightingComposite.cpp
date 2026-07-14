#include "../../PCH.h"
#include "Frame/Lighting/LightingComposite.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/LightingCompositePass.h"

void AddLightingCompositePass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle output,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer)
{
	auto& parameters = builder.AllocParameters<LightingCompositePass::Parameters>();
	parameters->SceneColor = builder.CreateUAV(output);
	parameters->DirectDiffuse = builder.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = builder.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateSRV(lighting.IndirectSpecular);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferEmissive = builder.CreateSRV(gbuffer.Emissive);
	builder.Dispatch<LightingCompositePass>(parameters);
}
