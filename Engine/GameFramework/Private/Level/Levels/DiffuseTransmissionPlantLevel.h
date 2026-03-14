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
		desc.initialCamera.transform = Transform({0.0f, 1.8f, -5.0f}, {-0.1f, 0.0f, 0.0f});
		desc.importedMeshRequests.push_back({"DiffuseTransmissionPlant/DiffuseTransmissionPlant.gltf"});
		return desc;
	}
};