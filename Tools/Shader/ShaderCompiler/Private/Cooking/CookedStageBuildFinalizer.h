#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/CookNode.h"

#include <string_view>

class CookedStageBuildFinalizer final
{
  public:
	CookedStageBuildFinalizer() = delete;

	static void ApplyNodeMetadata(
	    const CookNode& node,
	    std::string_view cacheStatus,
	    CookedStageBuild& compiledStage);
};