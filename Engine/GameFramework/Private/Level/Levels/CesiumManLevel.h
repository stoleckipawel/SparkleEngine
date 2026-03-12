#pragma once

#include "Level/Level.h"

class CesiumManLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "CesiumMan"; }

	std::string_view GetDescription() const override { return "Cesium Man — imported glTF character scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		desc.importedMeshRequests.push_back({"CesiumMan/CesiumMan.gltf"});
		return desc;
	}
};