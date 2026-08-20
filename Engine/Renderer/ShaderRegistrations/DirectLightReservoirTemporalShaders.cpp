#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

class DirectLightReservoirTemporalCS final : public TGlobalShader<DirectLightReservoirTemporalCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, TemporalReservoirSample, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, TemporalReservoirWeight, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirSample, PreviousReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirWeight, PreviousReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirSurface, PreviousReservoirSurfaceTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(Frame, FrameUniformData, FrameUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(View, ViewUniformData, ViewUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewCamera, ViewCameraUniformData, ViewCameraUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewTemporal, ViewTemporalUniformData, ViewTemporalUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(SceneLighting, SceneLighting, SceneLightingUniformData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightGpuData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightGpuData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightGpuData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightGpuData, RectLights)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferSubsurface)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMotionVector)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightReservoirTemporalCS,
    RendererShaderPackages::DirectLightReservoirTemporal,
    "Passes/Deferred/DirectLightReservoirTemporal.hlsl",
    "main",
    Compute);
