#pragma once

#include "Cooking/ShaderCookResult.h"
#include "Cooking/ShaderCookSettings.h"

class GlobalShaderCooker final
{
public:
	ShaderCookResult CookAll(const ShaderCookSettings& settings = {}) const;
};
