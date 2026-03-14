#pragma once

#include "Level/Level.h"

class SponzaLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "Sponza"; }

	std::string_view GetDescription() const override { return "Sponza Palace — standard PBR test scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		desc.initialCamera.transform = Transform({0.0f, 1.6f, -8.0f}, {0.0f, 0.0f, 0.0f});
		desc.importedMeshRequests.push_back({"Sponza/Sponza.gltf"});
		return desc;
	}
};
