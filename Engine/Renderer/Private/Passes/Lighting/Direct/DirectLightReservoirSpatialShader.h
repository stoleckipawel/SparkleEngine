#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

class DirectLightReservoirSpatialCS final : public GlobalShader<DirectLightReservoirSpatialCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TemporalReservoirSample)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TemporalReservoirWeight)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CurrentReservoirSample)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CurrentReservoirWeight)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CurrentReservoirSurface)
	SHADER_PARAMETER_CBUFFER(FrameUniformData, Frame)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	SHADER_PARAMETER_CBUFFER(SceneLightingUniformData, SceneLighting)
	SHADER_PARAMETER_BUFFER_SRV(DirectionalLightGpuData, DirectionalLights)
	SHADER_PARAMETER_BUFFER_SRV(PointLightGpuData, PointLights)
	SHADER_PARAMETER_BUFFER_SRV(SpotLightGpuData, SpotLights)
	SHADER_PARAMETER_BUFFER_SRV(RectLightGpuData, RectLights)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferSubsurface)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneDepth)
	END_SHADER_PARAMETER_STRUCT()
};
