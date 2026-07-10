#pragma once

#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

#define DIRECT_LIGHT_RESERVOIR_FRAME_SHADER_PARAMETERS                                               \
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData) \
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)

#define DIRECT_LIGHT_RESERVOIR_SURFACE_AND_LIGHTING_SHADER_PARAMETERS                      \
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)           \
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights) \
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)             \
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)               \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)               \
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)                                  \
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)                                     \
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)                                   \
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferSubsurface)                                 \
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
