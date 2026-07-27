#pragma once

#include <span>
#include <string>

struct CookedSceneBuild;

class CookedSceneGenerationWriter final
{
  public:
	static bool Publish(
	    std::span<const CookedSceneBuild* const> builds,
	    std::string& outErrorMessage);
};
