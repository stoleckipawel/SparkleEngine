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

	static Key TranslateVirtualKey(WPARAM VirtualKey) noexcept;

	static ModifierFlags GetCurrentModifiers() noexcept;
};
