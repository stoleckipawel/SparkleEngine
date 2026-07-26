#pragma once

#include <filesystem>
#include <string>
#include <vector>

class CatalogedLevelSceneReader final
{
  public:
	static bool AppendSceneIds(
	    const std::filesystem::path& levelPath,
	    std::vector<std::string>& outSceneIds,
	    std::string& outErrorMessage);

  private:
	static std::string ResolveSourceSceneId(std::string assetBinding);
};
