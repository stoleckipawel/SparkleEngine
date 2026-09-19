#include "../../PCH.h"
#include "Passes/Visualization/LightingVisualization.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Visualization/LightingVisualizationShader.h"
#include "View/RenderView.h"

static bool IsLightingVisualizationActive(RenderViewMode viewMode) noexcept
{
	switch (viewMode)
	{
		case RenderViewMode::DirectDiffuse:
		case RenderViewMode::DirectSpecular:
		case RenderViewMode::DirectSubsurface:
		case RenderViewMode::IndirectDiffuse:
		case RenderViewMode::IndirectSpecular:
			return true;
		default:
			return false;
	}
}

void AddLightingVisualizationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewMode viewMode,
    const RenderFrameGraphResources& resources)
{
	if (!IsLightingVisualizationActive(viewMode))
	{
		return;
	}

	const LightingRenderTargets& lighting = resources.Transient.Lighting;

	auto& parameters = builder.AllocParameters<LightingVisualizationCS>();
	parameters->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->DirectDiffuse = builder.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = builder.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateSRV(lighting.IndirectSpecular);

	builder.AddParameterSetup<RenderView>(parameters, [](auto& fields, const RenderView& view) { fields.View = view.uniform; });

	builder.Dispatch<LightingVisualizationCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
