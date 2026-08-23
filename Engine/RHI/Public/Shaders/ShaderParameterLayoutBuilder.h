#pragma once

#include "../RHIAPI.h"
#include "Authoring/GlobalShader.h"

#include <span>

class PassParameterLayout;

class SPARKLE_RHI_API ShaderParameterLayoutBuilder final
{
public:
	ShaderParameterLayoutBuilder() = delete;

	static PassParameterLayout Build(std::span<const ShaderRegistrationDesc* const> shaders);
};

SPARKLE_RHI_API PassParameterLayout BuildShaderParameterLayout(const ShaderRegistrationDesc& shader);
SPARKLE_RHI_API PassParameterLayout BuildShaderPipelineParameterLayout(std::span<const ShaderRegistrationDesc* const> shaders);
