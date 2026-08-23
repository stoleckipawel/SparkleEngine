#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/ViewUniformData.h"

class DirectLightingCS final : public GlobalShader<DirectLightingCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, DirectDiffuse)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, DirectSpecular)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, DirectSubsurface)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ShadowVisibilitySignal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, CurrentReservoirSample)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, CurrentReservoirWeight)
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

class FrameGraphBuilder;
struct DirectShadowSignalResources;
struct RenderFrameGraphImportedSceneResources;

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const RenderFrameGraphImportedSceneResources& externalResources);
