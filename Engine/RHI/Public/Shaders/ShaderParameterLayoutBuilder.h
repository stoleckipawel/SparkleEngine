#pragma once

#include "../RHIAPI.h"
#include "Authoring/GlobalShader.h"

#include <span>

class PassParameterLayout;

namespace ShaderParameterLayoutBuilder
{
	SPARKLE_RHI_API ShaderStageVisibility GetDefaultVisibility(ShaderStage stage) noexcept;
	SPARKLE_RHI_API PassParameterLayout Build(std::span<const ShaderRegistrationDesc* const> shaders);
}

SPARKLE_RHI_API PassParameterLayout BuildShaderParameterLayout(const ShaderRegistrationDesc& shader);
SPARKLE_RHI_API PassParameterLayout BuildShaderPipelineParameterLayout(std::span<const ShaderRegistrationDesc* const> shaders);
