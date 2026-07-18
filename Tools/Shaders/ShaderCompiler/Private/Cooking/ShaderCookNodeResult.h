#pragma once

#include "Cooking/CookedStageBuild.h"

#include <string>

struct ShaderCookNodeResult final
{
	CookedStageBuild CompiledStage;
	std::string Diagnostic;
	bool Succeeded = false;
	bool CacheHit = false;
	bool BackendInvoked = false;
};
