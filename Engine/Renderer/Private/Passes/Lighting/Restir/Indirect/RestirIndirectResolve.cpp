#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectResolve.h"

#include "Frame/RenderFrame.h"
#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectResolveShader.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectShaderParameters.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "ShaderData/SceneShaderParameters.h"

void AddRestirIndirectResolvePass(FrameGraphBuilder& builder, const RenderFrame& frame, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectResolveCS>();
	parameters->CurrentReservoirSampleTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Sample.Current);
	parameters->CurrentReservoirWeightTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Weight.Current);
	parameters->IndirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(resources.Transient.Lighting.IndirectSpecular);
	parameters->RayReconstructionDiffuseAlbedo = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.DiffuseAlbedo);
	parameters->RayReconstructionSpecularAlbedo = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.SpecularAlbedo);
	parameters->RayReconstructionRoughness = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.Roughness);
	parameters->RayReconstructionSpecularHitDistance = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.SpecularHitDistance);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(resources.Transient.GBuffer.WorldNormal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);

	BindSceneShaderParameters(builder, frame, parameters, resources);
	BindRayTracedShadowParameters(frame.PreparedScene, parameters);

	BindRestirIndirectParameters(parameters, resources);

	parameters->RestirIndirectWriteReconstructionGuides = ShouldUseRayReconstruction(frame.View.viewMode) ? 1u : 0u;

	builder.Dispatch<RestirIndirectResolveCS>(parameters, ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
