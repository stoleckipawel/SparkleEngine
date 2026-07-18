#pragma once

#include "Scene/SceneObjectSelection.h"
#include "World/WorldReadView.h"

#include <string>
#include <vector>

struct SceneOutlinerEntry
{
	std::string label;
	std::string typeLabel;
	SceneObjectSelection selection;
};

namespace SceneOutlinerEntries
{
	std::vector<SceneOutlinerEntry> BuildCameraEntries(const WorldReadView& readView);
	std::vector<SceneOutlinerEntry> BuildSkyEntries(const WorldReadView& readView);
	std::vector<SceneOutlinerEntry> BuildLightEntries(const WorldReadView& readView);
	std::vector<SceneOutlinerEntry> BuildMeshEntries(const WorldReadView& readView);
}  // namespace SceneOutlinerEntries
