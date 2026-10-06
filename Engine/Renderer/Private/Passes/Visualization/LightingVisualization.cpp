#include "../../PCH.h"
#include "Passes/Visualization/LightingVisualization.h"

#include "Frame/RenderFrame.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "ShaderData/SceneShaderParameters.h"
#include "Passes/Visualization/LightingVisualizationShader.h"

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
