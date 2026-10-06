#include "../../PCH.h"
#include "Passes/Visualization/GBufferVisualization.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Visualization/GBufferVisualizationShader.h"
#include "View/RenderView.h"

void AddGBufferVisualizationPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	switch (builder.GetViewMode())
	{
		case RenderViewMode::GBufferDiffuse:
		case RenderViewMode::GBufferWorldNormal:
		case RenderViewMode::GBufferWorldTangent:
		case RenderViewMode::GBufferRoughness:
		case RenderViewMode::GBufferMetallic:
		case RenderViewMode::GBufferEmissive:
		case RenderViewMode::GBufferAmbientOcclusion:
		case RenderViewMode::GBufferSubsurfaceColor:
		case RenderViewMode::GBufferSubsurfaceStrength:
			break;
		default:
			return;
	}

	const GBufferRenderTargets& gbuffer = resources.Transient.GBuffer;

	auto& parameters = builder.AllocParameters<GBufferVisualizationCS>();
	parameters->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(gbuffer.WorldNormal);
	parameters->GBufferWorldTangent = builder.CreateSRV(gbuffer.WorldTangent);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferEmissive = builder.CreateSRV(gbuffer.Emissive);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);

	builder.AddParameterSetup<RenderView>(parameters, [](auto& parameters, const RenderView& view) { parameters->View = view.uniform; });

	builder.Dispatch<GBufferVisualizationCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
