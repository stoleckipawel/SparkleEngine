#pragma once

#include "Level/Level.h"

class DiffuseTransmissionPlantLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "DiffuseTransmissionPlant"; }

	std::string_view GetDescription() const override { return "Diffuse Transmission Plant — imported glTF foliage test scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		desc.importedMeshRequests.push_back({"DiffuseTransmissionPlant/DiffuseTransmissionPlant.gltf"});
		return desc;
	}
};