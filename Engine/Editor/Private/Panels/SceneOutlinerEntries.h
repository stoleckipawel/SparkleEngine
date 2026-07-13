#pragma once

#include "Scene/SceneObjectSelection.h"

#include <string>
#include <vector>

class GameScene;

struct SceneOutlinerEntry
{
	std::string label;
	std::string typeLabel;
	SceneObjectSelection selection;
};

namespace SceneOutlinerEntries
{
	std::vector<SceneOutlinerEntry> BuildCameraEntries(const GameScene& gameScene);
	std::vector<SceneOutlinerEntry> BuildSkyEntries(const GameScene& gameScene);
	std::vector<SceneOutlinerEntry> BuildLightEntries(const GameScene& gameScene);
	std::vector<SceneOutlinerEntry> BuildMeshEntries(const GameScene& gameScene);
}  // namespace SceneOutlinerEntries
