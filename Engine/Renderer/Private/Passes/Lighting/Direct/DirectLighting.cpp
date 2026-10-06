#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLighting.h"

#include "Core/Public/Math/MathUtils.h"
#include "Passes/Lighting/Shadows/DirectShadowControls.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "ShaderData/SceneShaderParameters.h"

void AddDirectLightingPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	const LightingRenderTargets& lighting = resources.Transient.Lighting;
	const GBufferRenderTargets& gbuffer = resources.Transient.GBuffer;

	auto& parameters = builder.AllocParameters<DirectLightingCS>();
	parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
	RequireDirectShadowSignal(resources.Transient.ShadowVisibilitySignal.IsValid());
	parameters->ShadowVisibilitySignal = builder.CreateSRV(resources.Transient.ShadowVisibilitySignal);
	builder.AddResourceProductionSetup(
	    parameters,
	    resources.Transient.ShadowVisibilitySignal,
	    [](auto&, bool produced) { RequireDirectShadowSignal(produced); });
	parameters->CurrentReservoirSample = builder.CreateSRV(resources.History.DirectLightReservoir.Sample.Current);
	parameters->CurrentReservoirWeight = builder.CreateSRV(resources.History.DirectLightReservoir.Weight.Current);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(gbuffer.WorldNormal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);

	BindSceneShaderParameters(builder, parameters, resources);

	builder.AddPassParameterSetup(
	    parameters,
	    [baseColorInput = gbuffer.BaseColor,
	        materialInput = gbuffer.Material,
	        subsurfaceInput = gbuffer.Subsurface,
	        diffuseOutput = lighting.DirectDiffuse,
	        specularOutput = lighting.DirectSpecular,
	        subsurfaceOutput = lighting.DirectSubsurface](auto& parameters)
	    {
		    parameters->DirectLightingEvaluateDiffuse = IsDirectDiffuseActive(baseColorInput, diffuseOutput) ? 1u : 0u;
		    parameters->DirectLightingEvaluateSpecular = IsDirectSpecularActive(materialInput, specularOutput) ? 1u : 0u;
		    parameters->DirectLightingEvaluateSubsurface = IsDirectSubsurfaceActive(subsurfaceInput, subsurfaceOutput) ? 1u : 0u;
		    parameters->DirectLightingEvaluateShadows = IsDirectShadowsActive() ? 1u : 0u;
	    });

	builder.Dispatch<DirectLightingCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
