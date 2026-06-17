#pragma once

#include "EditorAPI.h"

class Window;

class SPARKLE_EDITOR_API EditorRestartService final
{
  public:
	bool Restart(Window& hostWindow) const;
};
