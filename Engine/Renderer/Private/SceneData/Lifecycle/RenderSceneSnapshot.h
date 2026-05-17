#pragma once

#include "GameFramework/Public/Scene/GameSceneSnapshot.h"

class GameScene;

struct RenderSceneSnapshot : GameSceneSnapshot
{
	void Capture(GameSceneSnapshot&& gameSceneSnapshot) noexcept;
};
