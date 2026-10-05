#pragma once

#include "Core/Public/Console/CVarControl.h"
#include "Renderer/Public/Viewport/RenderViewMode.h"

#include <string>

void DrawViewportShowMenu(const CVarControlExecutor* executor, RenderViewMode viewMode, bool disableInteraction, std::string& error);
