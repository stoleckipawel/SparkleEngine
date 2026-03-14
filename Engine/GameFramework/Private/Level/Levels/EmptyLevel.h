#pragma once

#include "Level/Level.h"

class EmptyLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "Empty"; }

	std::string_view GetDescription() const override { return "Empty level — blank canvas"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		desc.initialCamera.transform = Transform({0.0f, 0.0f, -4.0f}, {0.0f, 0.0f, 0.0f});
		return desc;
	}
};
