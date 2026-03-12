#pragma once

#include <cstdint>

enum class InputLayer : std::uint8_t
{
	System = 0,    // OS-level, always active (Alt+F4, screenshots)
	Console = 1,   // Debug console input
	Debug = 2,     // Profiler, debug overlays
	HUD = 3,       // UI elements (ImGui, menus, dialogs)
	Gameplay = 4,  // Player, camera, interaction

	Count
};
