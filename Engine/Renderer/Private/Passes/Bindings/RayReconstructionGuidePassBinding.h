#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

namespace RayReconstructionGuidePassBinding
{
	template <typename TParameters> void Describe(ShaderParameterStructBuilder<TParameters>& builder)
	{
		builder.RWTexture(
		    "RayReconstructionDiffuseAlbedo",
		    &TParameters::RayReconstructionDiffuseAlbedo,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "RayReconstructionSpecularAlbedo",
		    &TParameters::RayReconstructionSpecularAlbedo,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "RayReconstructionRoughness",
		    &TParameters::RayReconstructionRoughness,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "RayReconstructionSpecularHitDistance",
		    &TParameters::RayReconstructionSpecularHitDistance,
		    ShaderStageVisibility::Compute);
	}

	template <typename TParameters>
	void Bind(FrameGraphBuilder& builder, const LightingRenderTargets& lighting, TypedPassParameterInstance<TParameters>& parameters)
	{
		const LightingRenderTargets::RayReconstructionGuides& guides = lighting.ReconstructionGuides;
		parameters->RayReconstructionDiffuseAlbedo = builder.CreateUAV(guides.DiffuseAlbedo);
		parameters->RayReconstructionSpecularAlbedo = builder.CreateUAV(guides.SpecularAlbedo);
		parameters->RayReconstructionRoughness = builder.CreateUAV(guides.Roughness);
		parameters->RayReconstructionSpecularHitDistance = builder.CreateUAV(guides.SpecularHitDistance);
	}
}
