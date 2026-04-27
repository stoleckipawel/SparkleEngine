#pragma once

#include "EditorAPI.h"

class InputSystem;
class UI;

class SPARKLE_EDITOR_API EditorInputCoordinator final
{
  public:
	explicit EditorInputCoordinator(InputSystem& inputSystem) noexcept;
	~EditorInputCoordinator() noexcept;

	EditorInputCoordinator(const EditorInputCoordinator&) = delete;
	EditorInputCoordinator& operator=(const EditorInputCoordinator&) = delete;
	EditorInputCoordinator(EditorInputCoordinator&&) = delete;
	EditorInputCoordinator& operator=(EditorInputCoordinator&&) = delete;

	void UpdateGameplayInput(const UI& ui) noexcept;

  private:
	InputSystem* m_inputSystem = nullptr;
};
