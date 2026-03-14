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
		desc.initialCamera.transform = Transform({0.0f, 1.5f, -3.0f}, {0.0f, 0.0f, 0.0f});
		desc.importedMeshRequests.push_back({"CesiumMan/CesiumMan.gltf"});
		return desc;
	}
};