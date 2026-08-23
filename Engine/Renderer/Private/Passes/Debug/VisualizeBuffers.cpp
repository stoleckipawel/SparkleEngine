#include "../../PCH.h"
#include "Passes/Debug/VisualizeBuffers.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Debug/VisualizeBuffersShader.h"
#include "View/RenderView.h"

void AddVisualizeBuffersPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle resolvedSceneColor,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer)
{
	auto& parameters = builder.AllocParameters<VisualizeBuffersCS>();
	parameters->SceneColor = builder.CreateUAV(resolvedSceneColor);
	parameters->DirectDiffuse = builder.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = builder.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateSRV(lighting.IndirectSpecular);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferEmissive = builder.CreateSRV(gbuffer.Emissive);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	builder.AddParameterSetup<RenderView>(parameters, [](auto& fields, const RenderView& view) { fields.View = view.uniform; });
	builder.Dispatch<VisualizeBuffersCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
