// ============================================================================
// Win32InputBackend.h
// ----------------------------------------------------------------------------
// Windows implementation of IInputBackend.
// Translates WM_* messages to engine input events.
//
// ============================================================================

#pragma once

#include "IInputBackend.h"
#include "Input/Keyboard/Key.h"

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

class Win32InputBackend final : public IInputBackend
{
  public:
	InputBackendResult ProcessMessage(uint32_t Msg, uintptr_t Param1, intptr_t Param2) override;

	/// Translates Win32 VK_* to engine Key enum.
	static Key TranslateVirtualKey(WPARAM VirtualKey) noexcept;

	/// Gets current modifier state from Win32 GetKeyState().
	static ModifierFlags GetCurrentModifiers() noexcept;
};
