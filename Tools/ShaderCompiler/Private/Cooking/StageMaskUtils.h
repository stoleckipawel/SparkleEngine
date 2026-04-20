#pragma once

#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"

class StageMaskUtils final
{
  public:
	static ShaderStageMask FromVisibility(ShaderStageVisibility visibility) noexcept;
};
