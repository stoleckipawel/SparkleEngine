#pragma once

#include "Cooking/CookNode.h"
#include "Cooking/CookedStageBuild.h"

class IShaderBackend;
struct ShaderPackageCookSettings;

class ShaderCookNodeExecutor final
{
  public:
	ShaderCookNodeExecutor() = delete;
	static CookedStageBuild Execute(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node);

  private:
	static CookedStageBuild Compile(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    IShaderBackend& backend);
	static void ApplyNodeMetadata(const CookNode& node, CookedStageBuild& compiledStage);
};
