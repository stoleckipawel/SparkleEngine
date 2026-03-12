// =============================================================================
// InputLayer.h - Input Consumption Priority Layers
// =============================================================================
//
// Priority layers for input consumption. Lower values = higher priority.
// Higher priority layers can block lower ones.
//
// Layer ordering (highest to lowest priority):
//   System    -> OS-level shortcuts, cannot be blocked (Alt+F4, Alt+Tab)
//   Console   -> Debug console, command input
//   Debug     -> Profiler, debug overlays, dev tools
//   HUD       -> UI elements (ImGui, menus, dialogs)
//   Gameplay  -> Player control, camera, vehicles, interaction
//
#pragma once

#include <cstdint>

// =============================================================================
// InputLayer - Input Consumption Priority Layers
// =============================================================================

enum class InputLayer : std::uint8_t
{
	System = 0,    // OS-level, always active (Alt+F4, screenshots)
	Console = 1,   // Debug console input
	Debug = 2,     // Profiler, debug overlays
	HUD = 3,       // UI elements (ImGui, menus, dialogs)
	Gameplay = 4,  // Player, camera, interaction

	Count
};
