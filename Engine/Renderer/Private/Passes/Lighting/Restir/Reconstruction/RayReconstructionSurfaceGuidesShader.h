#pragma once

#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

class RayReconstructionSurfaceGuidesCS final : public GlobalShader<RayReconstructionSurfaceGuidesCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, RayReconstructionSurfaceGuidesCS)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferWorldNormal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneDepth)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, RayReconstructionDiffuseAlbedo)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, RayReconstructionSpecularAlbedo)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, RayReconstructionRoughness)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, RayReconstructionSpecularHitDistance)
	END_SHADER_PARAMETER_STRUCT()
};
