#pragma once

#include <span>

struct CookedSceneBuild;

class CookedSceneGenerationWriter final
{
  public:
	static void Publish(std::span<const CookedSceneBuild* const> builds);
};
