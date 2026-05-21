#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>
#include <string>

class ShaderBackendPool;
struct ShaderPackageCookSettings;

class ShaderCookNodeBuilder final
{
  public:
	ShaderCookNodeBuilder() = delete;

	static bool BuildAndAdd(
	    const ShaderPackageCookSettings& settings,
	    bool writeDebugArtifacts,
	    std::size_t packageIndex,
	    std::size_t stageIndex,
	    std::size_t targetIndex,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& plan,
	    std::string& outErrorMessage);
};