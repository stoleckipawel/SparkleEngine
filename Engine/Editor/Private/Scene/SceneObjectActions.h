#pragma once

#include "Scene/SceneObjectSelection.h"

class GameScene;

namespace SceneObjectActions
{
	bool IsSelectionValid(const GameScene& gameScene, const SceneObjectSelection& selection) noexcept;
	bool IsVisible(const GameScene& gameScene, const SceneObjectSelection& selection) noexcept;
	void ToggleVisibility(GameScene& gameScene, const SceneObjectSelection& selection) noexcept;
	void ApplySelection(GameScene& gameScene, const SceneObjectSelection& selection) noexcept;
}  // namespace SceneObjectActions
