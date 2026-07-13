#pragma once

#include <string>

class GameScene;

class SceneSkyInspector final
{
  public:
	static void Build(GameScene& gameScene, const std::string& filterText) noexcept;
};
