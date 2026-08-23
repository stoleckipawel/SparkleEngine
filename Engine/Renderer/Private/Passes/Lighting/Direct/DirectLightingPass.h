#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

#include <cstdint>

struct RenderPassDefinition;
struct ComputePassPipelineRuntime;
struct PassCommandContext;
struct DirectShadowSignalResources;

struct DirectLightingPassParameters
{
	ShaderRWTexture2D<void> DirectDiffuse;
	ShaderRWTexture2D<void> DirectSpecular;
	ShaderRWTexture2D<void> DirectSubsurface;
	ShaderTexture2D<void> ShadowVisibilitySignal;
	ShaderTexture2D<void> CurrentReservoirSample;
	ShaderTexture2D<void> CurrentReservoirWeight;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferSubsurface;
	ShaderTexture2D<void> SceneDepth;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;
	ShaderUniform<SceneLightingUniformData> SceneLighting;
	ShaderBuffer<void> DirectionalLights;
	ShaderBuffer<void> PointLights;
	ShaderBuffer<void> SpotLights;
	ShaderBuffer<void> RectLights;

	static void Describe(ShaderParameterStructBuilder<DirectLightingPassParameters>& builder)
	{
		builder.RWTexture("DirectDiffuse", &DirectLightingPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSpecular", &DirectLightingPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSubsurface", &DirectLightingPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "ShadowVisibilitySignal",
		    &DirectLightingPassParameters::ShadowVisibilitySignal,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "CurrentReservoirSample",
		    &DirectLightingPassParameters::CurrentReservoirSample,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "CurrentReservoirWeight",
		    &DirectLightingPassParameters::CurrentReservoirWeight,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &DirectLightingPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectLightingPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &DirectLightingPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferSubsurface", &DirectLightingPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("SceneDepth", &DirectLightingPassParameters::SceneDepth, ShaderStageVisibility::Compute);
		builder.Uniform("View", &DirectLightingPassParameters::View, ShaderStageVisibility::Compute);
		builder.Uniform("ViewCamera", &DirectLightingPassParameters::ViewCamera, ShaderStageVisibility::Compute);
		builder.Uniform("ViewTemporal", &DirectLightingPassParameters::ViewTemporal, ShaderStageVisibility::Compute);
		builder.Uniform("SceneLighting", &DirectLightingPassParameters::SceneLighting, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectLightingPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectLightingPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectLightingPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectLightingPassParameters::RectLights, ShaderStageVisibility::Compute);
	}
};

class DirectLightingPass final
{
public:
	static constexpr const char* PassName = "DirectLighting";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectLightingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};
