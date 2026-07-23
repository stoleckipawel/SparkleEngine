#pragma once

#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/ShaderReflection.h"
#include "Shaders/ShaderStage.h"

namespace CookedShaderBindingRules
{
	bool HasAllStages(ShaderStageMask value, ShaderStageMask flags) noexcept;
	ShaderStageMask ToPackageStageMask(ShaderStageVisibility visibility) noexcept;
	bool ResourceKindMatchesSemantic(
	    CookedShaderResourceKind resourceKind,
	    ShaderParameterSemanticKind semanticKind) noexcept;
}
