#pragma once

#include "Cooking/ShaderCookResult.h"
#include "Cooking/ShaderCookSettings.h"

class ShaderPackageCooker final
{
  public:
	ShaderPackageCookResult CookAll(const ShaderPackageCookSettings& settings = {}) const;
};
