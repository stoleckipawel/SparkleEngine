#pragma once

#include "Level/Level.h"

class DamagedHelmetLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "DamagedHelmet"; }

	std::string_view GetDescription() const override { return "Damaged Helmet — imported glTF material test scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		desc.importedMeshRequests.push_back({"DamagedHelmet/DamagedHelmet.gltf"});
		return desc;
	}
};