#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirIndirectResolve.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectResolveShader.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "ShaderData/SceneShaderParameters.h"

void AddRestirIndirectResolvePass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectResolveCS>();
	parameters->CurrentReservoirSampleTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Sample.Current);
	parameters->CurrentReservoirWeightTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Weight.Current);
	parameters->IndirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(resources.Transient.Lighting.IndirectSpecular);
	parameters->RayReconstructionDiffuseAlbedo = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.DiffuseAlbedo);
	parameters->RayReconstructionSpecularAlbedo = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.SpecularAlbedo);
	parameters->RayReconstructionRoughness = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.Roughness);
	parameters->RayReconstructionSpecularHitDistance =
	    builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.SpecularHitDistance);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(resources.Transient.GBuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);

	BindSceneShaderParameters(builder, parameters, resources);
	BindRayTracedShadowParameters(builder, parameters);

	builder.AddPassParameterSetup(
	    parameters,
	    [](auto& fields)
	    {
		    const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
		    fields.RestirIndirectConstants = RestirIndirectLightingUniformData{.BounceCount = settings.BounceCount};
	    });

	builder.Dispatch<RestirIndirectResolveCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
