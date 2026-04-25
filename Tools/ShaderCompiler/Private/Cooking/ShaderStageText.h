#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

#include <string>

class ShaderStageText final
{
  public:
	ShaderStageText() = delete;

	static std::string FormatMask(ShaderStageMask mask);
};