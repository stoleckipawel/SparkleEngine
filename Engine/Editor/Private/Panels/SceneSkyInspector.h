#pragma once

#include <string>

class GameWorld;

class SceneSkyInspector final
{
  public:
	static void Build(GameWorld& gameWorld, const std::string& filterText) noexcept;
};
