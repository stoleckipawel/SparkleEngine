#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include "ShaderData/FrameUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

#include <type_traits>

struct DirectLightReservoirCommonParameters
{
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferSubsurface;
	ShaderTexture2D<void> SceneDepth;
	ShaderUniform<FrameUniformData> Frame;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<SceneLightingUniformData> SceneLighting;
	ShaderBuffer<void> DirectionalLights;
	ShaderBuffer<void> PointLights;
	ShaderBuffer<void> SpotLights;
	ShaderBuffer<void> RectLights;

	template <typename TParameters> static void DescribeGBuffer(ShaderParameterStructBuilder<TParameters>& builder)
	{
		builder.ReadTexture(
		    "GBufferBaseColor",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::GBufferBaseColor),
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferNormal",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::GBufferNormal),
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferMaterial",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::GBufferMaterial),
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferSubsurface",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::GBufferSubsurface),
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "SceneDepth",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::SceneDepth),
		    ShaderStageVisibility::Compute);
	}

	template <typename TParameters> static void DescribeFrame(ShaderParameterStructBuilder<TParameters>& builder)
	{
		builder.Uniform("Frame", Member<TParameters>(&DirectLightReservoirCommonParameters::Frame), ShaderStageVisibility::Compute);
		builder.Uniform("View", Member<TParameters>(&DirectLightReservoirCommonParameters::View), ShaderStageVisibility::Compute);
		builder.Uniform(
		    "ViewCamera",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::ViewCamera),
		    ShaderStageVisibility::Compute);
	}

	template <typename TParameters> static void DescribeLighting(ShaderParameterStructBuilder<TParameters>& builder)
	{
		builder.Uniform(
		    "SceneLighting",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::SceneLighting),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "DirectionalLights",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::DirectionalLights),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "PointLights",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::PointLights),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "SpotLights",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::SpotLights),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "RectLights",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::RectLights),
		    ShaderStageVisibility::Compute);
	}

private:
	template <typename TParameters, typename TField>
	static constexpr auto Member(TField DirectLightReservoirCommonParameters::* member) noexcept -> TField TParameters::*
	{
		static_assert(std::is_base_of_v<DirectLightReservoirCommonParameters, TParameters>);
		return static_cast<TField TParameters::*>(member);
	}
};
