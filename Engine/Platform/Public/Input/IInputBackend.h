// ============================================================================
// IInputBackend.h
// ----------------------------------------------------------------------------
// Platform abstraction for translating native messages to input events.
//
#pragma once

#include "Platform/Public/PlatformAPI.h"

#include "Input/Events/KeyboardEvent.h"
#include "Input/Events/MouseButtonEvent.h"
#include "Input/Events/MouseMoveEvent.h"
#include "Input/Events/MouseWheelEvent.h"

#include <cstdint>

// ============================================================================
// InputBackendResult
// ============================================================================

/// Discriminator for InputBackendResult.
enum class InputEventType : uint8_t
{
	None,
	Keyboard,
	MouseButton,
	MouseMove,
	MouseWheel
};

struct InputBackendResult
{
	InputEventType Type = InputEventType::None;

	union
	{
		KeyboardEvent Keyboard;
		MouseButtonEvent MouseButton;
		MouseMoveEvent MouseMove;
		MouseWheelEvent MouseWheel;
	};

	InputBackendResult() : Keyboard{} {}

	bool IsValid() const noexcept { return Type != InputEventType::None; }
};

// ============================================================================
// IInputBackend
// ============================================================================

class SPARKLE_PLATFORM_API IInputBackend
{
  public:
	virtual ~IInputBackend() = default;

	/// Translates a native message to an engine input event.
	virtual InputBackendResult ProcessMessage(uint32_t Msg, uintptr_t Param1, intptr_t Param2) = 0;

  protected:
	IInputBackend() = default;
	IInputBackend(const IInputBackend&) = default;
	IInputBackend& operator=(const IInputBackend&) = default;
};
