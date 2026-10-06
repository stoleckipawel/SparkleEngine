#include "../../PCH.h"
#include "Passes/Visualization/LightingVisualization.h"

#include "Frame/RenderFrame.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "ShaderData/SceneShaderParameters.h"
#include "Passes/Visualization/LightingVisualizationShader.h"

bool PrepareLightingVisualizationProducts(
    RenderViewMode viewMode,
    const RenderFrameGraphResources& resources,
    ViewportFrameProducts& products) noexcept
{
	const auto& lighting = resources.Transient.Lighting;
	const auto& gbuffer = resources.Transient.GBuffer;
	bool active = true;
	switch (viewMode)
	{
		case RenderViewMode::DirectDiffuse:
			active = IsDirectDiffuseActive(gbuffer.BaseColor, lighting.DirectDiffuse);
			break;
		case RenderViewMode::DirectSpecular:
			active = IsDirectSpecularActive(gbuffer.Material, lighting.DirectSpecular);
			break;
		case RenderViewMode::DirectSubsurface:
			active = IsDirectSubsurfaceActive(gbuffer.Subsurface, lighting.DirectSubsurface);
			break;
		case RenderViewMode::IndirectDiffuse:
			active = IsIndirectDiffuseActive(gbuffer.BaseColor, lighting.IndirectDiffuse);
			break;
		case RenderViewMode::IndirectSpecular:
			active = IsIndirectSpecularActive(gbuffer.Material, lighting.IndirectSpecular);
			break;
		default:
			break;
	}
	if (active)
	{
		return true;
	}
	products = {};
	products.Progress.State = ViewportRenderProgressState::Unavailable;
	products.Progress.Reason = ViewportRenderProgressReason::FeatureDisabled;
	return false;
}

void AddLightingVisualizationPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	const auto& lighting = resources.Transient.Lighting;
	auto& parameters = builder.AllocParameters<LightingVisualizationCS>();
	parameters->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->DirectDiffuse = builder.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = builder.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateSRV(lighting.IndirectSpecular);
	BindSceneShaderParameters(builder, frame, parameters, resources);
	builder.Dispatch<LightingVisualizationCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
