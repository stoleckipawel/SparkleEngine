#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectResolve.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectResolveShader.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "ShaderData/SceneShaderParameters.h"

void AddRestirIndirectResolvePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool writeRayReconstructionGuides,
    const RenderFrameGraphResources& resources)
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
	        specular = lighting.IndirectSpecular,
	        writeRayReconstructionGuides](auto& fields)
	    {
		    auto uniform = BuildIndirectLightingUniform(baseColor, material, diffuse, specular);
		    uniform.WriteReconstructionGuides = writeRayReconstructionGuides ? 1u : 0u;
		    fields.RestirIndirectConstants = uniform;
	    });

	builder.Dispatch<RestirIndirectResolveCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
