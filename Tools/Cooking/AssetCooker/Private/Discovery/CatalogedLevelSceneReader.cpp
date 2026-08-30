#include "CatalogedLevelSceneReader.h"

#include "Core/Public/Strings/StringUtils.h"

#include <fstream>
#include <utility>

bool CatalogedLevelSceneReader::AppendSceneIds(
    const std::filesystem::path& levelPath,
    std::vector<std::string>& outSceneIds,
    std::string& outErrorMessage)
{
	std::ifstream input(levelPath);
	if (!input.is_open())
	{
		outErrorMessage = "Cataloged level file was not found: " + levelPath.string();
		return false;
	}

	bool inSceneAssetsSection = false;
	for (std::string line; std::getline(input, line);)
	{
		line = Strings::TrimCopy(line);
		if (line.empty() || line.front() == '#' || line.front() == ';')
		{
			continue;
		}

		if (line.front() == '[' && line.back() == ']')
		{
			inSceneAssetsSection = line == "[SceneAssets]";
			continue;
		}

		if (!inSceneAssetsSection)
		{
			continue;
		}

		std::string_view key;
		std::string_view value;
		if (!Strings::TrySplitKeyValue(line, '=', key, value) || key != "Asset")
		{
			continue;
		}

		std::string sceneId = ResolveSourceSceneId(Strings::UnquoteCopy(value));
		if (!sceneId.empty())
		{
			outSceneIds.push_back(std::move(sceneId));
		}
	}

	outErrorMessage.clear();
	return true;
}

std::string CatalogedLevelSceneReader::ResolveSourceSceneId(std::string assetBinding)
{
	const std::size_t sourceSeparator = assetBinding.rfind('|');
	if (sourceSeparator != std::string::npos)
	{
		assetBinding.erase(0, sourceSeparator + 1);
	}

	return Strings::TrimCopy(assetBinding);
}
