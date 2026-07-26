#pragma once

#include "CookedSceneBuild.h"
#include "Core/Public/Files/FileUtils.h"

#include <span>
#include <string>
#include <vector>

class CookedSceneGenerationWriter final
{
  public:
	static bool Publish(
	    std::span<const CookedSceneBuild* const> builds,
	    std::string& outErrorMessage);

  private:
	static bool StageAssets(
	    std::span<const CookedSceneBuild* const> builds,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static void Cleanup(std::span<const Files::FilePublication> publication) noexcept;
};
