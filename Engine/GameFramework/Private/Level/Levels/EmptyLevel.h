#pragma once

#include "Level/Level.h"

class EmptyLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "Empty"; }

	std::string_view GetDescription() const override { return "Empty level — blank canvas"; }

	LevelDesc BuildDescription() const override { return {}; }
};
