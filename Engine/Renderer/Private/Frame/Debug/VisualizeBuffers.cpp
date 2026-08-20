#include "../../PCH.h"
#include "Frame/Debug/VisualizeBuffers.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Debug/VisualizeBuffersPass.h"
#include "View/RenderView.h"

void AddVisualizeBuffersPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer)
{
	auto& parameters = builder.AllocParameters<VisualizeBuffersPass::Parameters>();
	auto* parameterFields = parameters.operator->();
	parameters->SceneColor = builder.CreateUAV(sceneTargets.FinalSceneColor);
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
	builder.AddRenderViewSetup([parameterFields](const RenderView& view) { parameterFields->View = view.uniform; });
	builder.Dispatch<VisualizeBuffersPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
