#include "../../PCH.h"
#include "Passes/Lighting/LightingComposite.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/LightingCompositeShader.h"

void AddLightingCompositePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle output,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer)
{
	auto& parameters = builder.AllocParameters<LightingCompositeCS>();
	parameters->SceneColor = builder.CreateUAV(output);
	parameters->DirectDiffuse = builder.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = builder.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateSRV(lighting.IndirectSpecular);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferEmissive = builder.CreateSRV(gbuffer.Emissive);
	builder.Dispatch<LightingCompositeCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
