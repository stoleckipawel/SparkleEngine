#pragma once

#include "Level/Level.h"

class ABeautifulGameLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "ABeautifulGame"; }

	std::string_view GetDescription() const override { return "A Beautiful Game — imported glTF showcase scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		desc.importedMeshRequests.push_back({"ABeautifulGame/ABeautifulGame.gltf"});
		return desc;
	}
};