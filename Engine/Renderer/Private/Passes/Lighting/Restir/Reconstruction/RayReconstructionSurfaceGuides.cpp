#include "PCH.h"
#include "Passes/Lighting/Restir/Reconstruction/RayReconstructionSurfaceGuides.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderFrame.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "Passes/Lighting/Restir/Reconstruction/RayReconstructionSurfaceGuidesShader.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstructionResources.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "ShaderData/SceneShaderParameters.h"

void AddRayReconstructionSurfaceGuidesPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	if (!ShouldUseRayReconstruction(frame.View.viewMode) || IsIndirectLightingAdmitted())
	{
		return;
	}
	CreateRayReconstructionGuideRenderTargets(builder, sceneExtent, resources);
	const auto& gbuffer = resources.Transient.GBuffer;
	const auto& guides = resources.Transient.Lighting.ReconstructionGuides;
	auto& parameters = builder.AllocParameters<RayReconstructionSurfaceGuidesCS>();
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(gbuffer.WorldNormal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	parameters->RayReconstructionDiffuseAlbedo = builder.CreateUAV(guides.DiffuseAlbedo);
	parameters->RayReconstructionSpecularAlbedo = builder.CreateUAV(guides.SpecularAlbedo);
	parameters->RayReconstructionRoughness = builder.CreateUAV(guides.Roughness);
	parameters->RayReconstructionSpecularHitDistance = builder.CreateUAV(guides.SpecularHitDistance);
	BindSceneShaderParameters(builder, frame, parameters, resources);
	builder.Dispatch<RayReconstructionSurfaceGuidesCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
