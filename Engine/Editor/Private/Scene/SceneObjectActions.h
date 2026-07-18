#pragma once

#include "Scene/SceneObjectSelection.h"

class GameWorld;

namespace SceneObjectActions
{
	bool IsSelectionValid(const GameWorld& gameWorld, const SceneObjectSelection& selection) noexcept;
	bool IsVisible(const GameWorld& gameWorld, const SceneObjectSelection& selection) noexcept;
	void ToggleVisibility(GameWorld& gameWorld, const SceneObjectSelection& selection) noexcept;
	void ApplySelection(GameWorld& gameWorld, const SceneObjectSelection& selection) noexcept;
}  // namespace SceneObjectActions
