#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

namespace IndirectLightingOutputPassBinding
{
	template <typename TParameters> void Describe(ShaderParameterStructBuilder<TParameters>& builder)
	{
#define DESCRIBE_INDIRECT_LIGHTING_OUTPUT(Name) builder.RWTexture(#Name, &TParameters::Name, ShaderStageVisibility::Compute)
		DESCRIBE_INDIRECT_LIGHTING_OUTPUT(IndirectDiffuse);
		DESCRIBE_INDIRECT_LIGHTING_OUTPUT(IndirectSpecular);
#undef DESCRIBE_INDIRECT_LIGHTING_OUTPUT
	}

	template <typename TParameters>
	void Bind(FrameGraphBuilder& builder, const LightingRenderTargets& lighting, TypedPassParameterInstance<TParameters>& parameters)
	{
#define BIND_INDIRECT_LIGHTING_OUTPUT(Name) parameters->Name = builder.CreateUAV(lighting.Name)
		BIND_INDIRECT_LIGHTING_OUTPUT(IndirectDiffuse);
		BIND_INDIRECT_LIGHTING_OUTPUT(IndirectSpecular);
#undef BIND_INDIRECT_LIGHTING_OUTPUT
	}
}
