#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectSpatial.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectSpatialShader.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "ShaderData/SceneShaderParameters.h"

void AddRestirIndirectSpatialPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectSpatialCS>();
	parameters->TemporalReservoirSampleTexture = builder.CreateSRV(workingReservoirs.TemporalSample);
	parameters->TemporalReservoirWeightTexture = builder.CreateSRV(workingReservoirs.TemporalWeight);
	parameters->CurrentReservoirSampleTexture = builder.CreateUAV(resources.History.RestirIndirectReservoir.Sample.Current);
	parameters->CurrentReservoirWeightTexture = builder.CreateUAV(resources.History.RestirIndirectReservoir.Weight.Current);
	parameters->CurrentReservoirSurfaceTexture = builder.CreateUAV(resources.History.RestirIndirectReservoir.Surface.Current);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(resources.Transient.GBuffer.WorldNormal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);

	BindSceneShaderParameters(builder, parameters, resources);
	BindRayTracedShadowParameters(builder, parameters);

	const auto& gbuffer = resources.Transient.GBuffer;
	const auto& lighting = resources.Transient.Lighting;
	builder.AddPassParameterSetup(
	    parameters,
	    [baseColor = gbuffer.BaseColor,
	        material = gbuffer.Material,
	        diffuse = lighting.IndirectDiffuse,
	        specular = lighting.IndirectSpecular](auto& parameters)
	    {
		    parameters->RestirIndirectBounceCount = ResolveRestirIndirectBounceCount();
		    parameters->RestirIndirectTemporalReuse = CVarRestirIndirectTemporalReuse.Get() ? 1u : 0u;
		    parameters->RestirIndirectSpatialReuse = CVarRestirIndirectSpatialReuse.Get() ? 1u : 0u;
		    parameters->RestirIndirectEvaluateDiffuse = IsIndirectDiffuseActive(baseColor, diffuse) ? 1u : 0u;
		    parameters->RestirIndirectEvaluateSpecular = IsIndirectSpecularActive(material, specular) ? 1u : 0u;
		    parameters->RestirIndirectTraceSecondaryShadows = IsIndirectShadowsActive() ? 1u : 0u;
	    });

	builder.Dispatch<RestirIndirectSpatialCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
