#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

#include <type_traits>

struct FrameContext;
struct PassRuntimeContext;
struct RenderViewData;

struct DirectLightReservoirCommonParameters
{
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferSubsurface;
	ShaderTexture2D<void> SceneDepth;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<ViewLightingData> ViewLighting;
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
		builder.Uniform("PerFrame", Member<TParameters>(&DirectLightReservoirCommonParameters::PerFrame), ShaderStageVisibility::Compute);
		builder.Uniform("PerView", Member<TParameters>(&DirectLightReservoirCommonParameters::PerView), ShaderStageVisibility::Compute);
	}

	template <typename TParameters> static void DescribeLighting(ShaderParameterStructBuilder<TParameters>& builder)
	{
		builder.Uniform(
		    "ViewLighting",
		    Member<TParameters>(&DirectLightReservoirCommonParameters::ViewLighting),
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

namespace DirectLightReservoirPassCommon
{
	void SetParameters(
	    DirectLightReservoirCommonParameters& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeContext& passRuntimeContext);
}
