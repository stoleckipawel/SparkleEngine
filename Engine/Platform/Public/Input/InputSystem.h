// ============================================================================
// InputSystem.h
// ----------------------------------------------------------------------------
// Central hub for all input processing.
//
// RESPONSIBILITIES:
//   - Owns the platform backend (IInputBackend)
//   - Owns the pollable InputState
//   - Routes window messages to backend for translation
//   - Updates InputState from translated events
//   - Manages event callbacks (immediate and deferred)
//   - Handles input layer priority and filtering
//   - Controls mouse capture and cursor visibility
//
// FRAME LIFECYCLE:
//   1. InputSystem.BeginFrame()        — Transitions button states, clears deltas
//   2. Window.PumpMessages()           — Window broadcasts OnWindowMessage event
//   3. InputSystem.ProcessDeferredEvents() — Fires deferred callbacks
//   4. GameCamera.Update()             — Polls InputState
//   5. InputSystem.EndFrame()          — Optional cleanup
//
// THREADING:
//   Single-threaded. All methods must be called from the main thread.
//
// EXAMPLE:
//   auto backend = std::make_unique<Win32InputBackend>();
//   InputSystem input(std::move(backend));
//
//   // Subscribe to window messages (decoupled from Window)
//   input.SubscribeToWindow(window);
//
//   auto handle = input.SubscribeKeyboard([](const KeyboardEvent& e) {
//       if (e.KeyCode == Key::Escape && e.bPressed) { QuitApp(); }
//   }, InputLayer::Gameplay);
//
//   // In frame loop:
//   input.BeginFrame();
//   window.PumpMessages();  // Window broadcasts messages, InputSystem receives them
//   input.ProcessDeferredEvents();
//   if (input.GetState().IsKeyHeld(Key::W)) { MoveForward(dt); }
//   input.EndFrame();
//
// ============================================================================

#pragma once

#include "Platform/Public/PlatformAPI.h"

#include "IInputBackend.h"
#include "Events/Event.h"
#include "Events/EventHandle.h"
#include "Events/ScopedEventHandle.h"
#include "Input/InputState.h"
#include "Input/Dispatch/InputLayer.h"
#include "Input/Dispatch/DispatchMode.h"
#include "Input/Events/KeyboardEvent.h"
#include "Input/Events/MouseButtonEvent.h"
#include "Input/Events/MouseMoveEvent.h"
#include "Input/Events/MouseWheelEvent.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

// Forward declarations
class Window;
struct WindowMessageEvent;

// ============================================================================
// Type list for all supported input event types
// ============================================================================

using InputEventTypes = std::tuple<KeyboardEvent, MouseButtonEvent, MouseMoveEvent, MouseWheelEvent>;

// ============================================================================
// Callback Type Aliases
// ============================================================================

template <typename TEvent> using InputCallback = std::function<void(const TEvent&)>;

using KeyboardCallback = InputCallback<KeyboardEvent>;
using MouseButtonCallback = InputCallback<MouseButtonEvent>;
using MouseMoveCallback = InputCallback<MouseMoveEvent>;
using MouseWheelCallback = InputCallback<MouseWheelEvent>;

// ============================================================================
// InputSystem
// ============================================================================

class SPARKLE_PLATFORM_API InputSystem
{
  public:
	// =========================================================================
	// Factory
	// =========================================================================

	/// Creates an InputSystem with the appropriate platform backend auto-detected.
	static std::unique_ptr<InputSystem> Create();

	// =========================================================================
	// Construction
	// =========================================================================

	/// Constructs the input system with a platform backend.
	/// @param Backend Platform-specific message translator (ownership transferred)
	explicit InputSystem(std::unique_ptr<IInputBackend> Backend);

	~InputSystem();

	// Non-copyable
	InputSystem(const InputSystem&) = delete;
	InputSystem& operator=(const InputSystem&) = delete;

	// Non-movable (due to mutex and callbacks)
	InputSystem(InputSystem&&) = delete;
	InputSystem& operator=(InputSystem&&) = delete;

	// =========================================================================
	// Frame Lifecycle
	// =========================================================================

	/// Call at the start of each frame.
	/// Polls window events, processes input, and fires callbacks.
	void BeginFrame();

	/// Call at the end of each frame for cleanup.
	void EndFrame();

	/// Processes all deferred events queued during message processing.
	/// Call after window message pumping and before gameplay updates.
	void ProcessDeferredEvents();

	// =========================================================================
	// Window Integration
	// =========================================================================

	/// Subscribes to a Window's message events for input processing.
	/// Uses the observer pattern - InputSystem receives messages via Window's event.
	/// @param window The window to subscribe to
	void SubscribeToWindow(Window& window);

	// =========================================================================
	// Message Processing
	// =========================================================================

	/// Handles a window message event (called from Window's OnWindowMessage event).
	/// @param event The window message event data
	void HandleWindowMessage(WindowMessageEvent& event);

	/// Processes a window message through the backend (legacy direct call).
	/// @param Msg    Windows message ID (WM_*)
	/// @param Param1 WPARAM
	/// @param Param2 LPARAM
	/// @return True if the message was handled as an input event
	bool OnWindowMessage(uint32_t Msg, uintptr_t Param1, intptr_t Param2);

	// =========================================================================
	// State Access
	// =========================================================================

	/// Returns the current pollable input state (read-only).
	const InputState& GetState() const noexcept { return m_State; }

	// =========================================================================
	// Layer Control
	// =========================================================================

	/// Enables or disables an input layer.
	/// Disabled layers do not receive callbacks (except System layer).
	void SetLayerEnabled(InputLayer Layer, bool bEnabled);

	/// Returns true if the specified layer is enabled.
	bool IsLayerEnabled(InputLayer Layer) const noexcept;

	/// Returns the highest priority (lowest value) enabled layer.
	InputLayer GetActiveLayer() const noexcept;

	// =========================================================================
	// Callback Subscription
	// =========================================================================

	/// Subscribes to keyboard events.
	/// @param Callback Function to call on keyboard events
	/// @param Layer    Input layer for priority filtering
	/// @param Mode     Immediate (in WndProc) or Deferred (ProcessDeferredEvents)
	/// @return Handle for unsubscription
	EventHandle SubscribeKeyboard(
	    KeyboardCallback Callback,
	    InputLayer Layer = InputLayer::Gameplay,
	    DispatchMode Mode = DispatchMode::Immediate);

	/// Subscribes to mouse button events.
	EventHandle SubscribeMouseButton(
	    MouseButtonCallback Callback,
	    InputLayer Layer = InputLayer::Gameplay,
	    DispatchMode Mode = DispatchMode::Immediate);

	/// Subscribes to mouse move events.
	EventHandle SubscribeMouseMove(
	    MouseMoveCallback Callback,
	    InputLayer Layer = InputLayer::Gameplay,
	    DispatchMode Mode = DispatchMode::Immediate);

	/// Subscribes to mouse wheel events.
	EventHandle SubscribeMouseWheel(
	    MouseWheelCallback Callback,
	    InputLayer Layer = InputLayer::Gameplay,
	    DispatchMode Mode = DispatchMode::Immediate);

	/// Unsubscribes a callback by handle.
	/// Safe to call with invalid handle (no-op).
	void Unsubscribe(EventHandle Handle);

	// =========================================================================
	// Public Events (for decoupled subscription)
	// =========================================================================

	/// Fired when a key is pressed.
	Event<void(const KeyboardEvent&)> OnKeyPressed;

	/// Fired when a key is released.
	Event<void(const KeyboardEvent&)> OnKeyReleased;

	/// Fired when a mouse button is pressed.
	Event<void(const MouseButtonEvent&)> OnMouseButtonPressed;

	/// Fired when a mouse button is released.
	Event<void(const MouseButtonEvent&)> OnMouseButtonReleased;

	/// Fired when the mouse moves.
	Event<void(const MouseMoveEvent&)> OnMouseMove;

	/// Fired when the mouse wheel scrolls.
	Event<void(const MouseWheelEvent&)> OnMouseWheel;

	// =========================================================================
	// Mouse Capture Control
	// =========================================================================

	/// Captures the mouse to the window (for drag operations, look control).
	void CaptureMouse();

	/// Releases mouse capture.
	void ReleaseMouse();

	/// Returns true if mouse is currently captured.
	bool IsMouseCaptured() const noexcept;

	/// Hides the cursor.
	void HideCursor();

	/// Shows the cursor.
	void ShowCursor();

	/// Returns true if cursor is currently hidden.
	bool IsCursorHidden() const noexcept;

	/// Sets cursor visibility (alias for Show/HideCursor).
	void SetCursorVisibility(bool bVisible);

	/// Centers the cursor in the given window. Used during mouse look to allow infinite movement.
	/// Call this each frame while mouse look is active to prevent cursor hitting screen edges.
	void CenterCursor(void* windowHandle);

  private:
	// =========================================================================
	// Callback Entry
	// =========================================================================

	template <typename TEvent> struct CallbackEntry
	{
		std::function<void(const TEvent&)> Callback;
		EventHandle Handle;
		InputLayer Layer = InputLayer::Gameplay;
		DispatchMode Mode = DispatchMode::Immediate;
	};

	// =========================================================================
	// Tuple types for callback storage and deferred queues
	// =========================================================================

	using CallbackTuple = std::tuple<
	    std::vector<CallbackEntry<KeyboardEvent>>,
	    std::vector<CallbackEntry<MouseButtonEvent>>,
	    std::vector<CallbackEntry<MouseMoveEvent>>,
	    std::vector<CallbackEntry<MouseWheelEvent>>>;

	using DeferredQueueTuple =
	    std::tuple<std::vector<KeyboardEvent>, std::vector<MouseButtonEvent>, std::vector<MouseMoveEvent>, std::vector<MouseWheelEvent>>;

	// =========================================================================
	// Internal Methods
	// =========================================================================

	/// Generates a unique callback handle ID.
	uint32_t GenerateCallbackId();

	/// Checks if a callback should fire based on layer state.
	bool ShouldDispatchToLayer(InputLayer Layer) const noexcept;

	// -------------------------------------------------------------------------
	// Tuple Accessors
	// -------------------------------------------------------------------------

	/// Gets the callback vector for a specific event type.
	template <typename TEvent> std::vector<CallbackEntry<TEvent>>& GetCallbacks();

	template <typename TEvent> const std::vector<CallbackEntry<TEvent>>& GetCallbacks() const;

	/// Gets the deferred queue for a specific event type.
	template <typename TEvent> std::vector<TEvent>& GetDeferredQueue();

	// -------------------------------------------------------------------------
	// Template Dispatch Helpers (reduces code duplication)
	// -------------------------------------------------------------------------

	/// Dispatches an event to all callbacks matching the specified mode.
	template <typename TEvent> void DispatchToCallbacks(const TEvent& Event, DispatchMode TargetMode);

	/// Queues event if any callbacks want deferred delivery.
	template <typename TEvent> void QueueIfHasDeferredCallbacks(const TEvent& Event);

	/// Processes an event through the full pipeline (state update + dispatch + queue).
	template <typename TEvent> void ProcessEvent(const TEvent& Event);

	/// Processes all deferred events for a specific event type.
	template <typename TEvent> void ProcessDeferredEventsForType();

	// -------------------------------------------------------------------------
	// State Update (overloads for each event type)
	// -------------------------------------------------------------------------

	void UpdateStateFromEvent(const KeyboardEvent& Event);
	void UpdateStateFromEvent(const MouseButtonEvent& Event);
	void UpdateStateFromEvent(const MouseMoveEvent& Event);
	void UpdateStateFromEvent(const MouseWheelEvent& Event);

	// -------------------------------------------------------------------------
	// Public Event Broadcasting (broadcasts to public Event<> members)
	// -------------------------------------------------------------------------

	void BroadcastToPublicEvent(const KeyboardEvent& Event);
	void BroadcastToPublicEvent(const MouseButtonEvent& Event);
	void BroadcastToPublicEvent(const MouseMoveEvent& Event);
	void BroadcastToPublicEvent(const MouseWheelEvent& Event);

	// -------------------------------------------------------------------------
	// Tuple Operations
	// -------------------------------------------------------------------------

	/// Clears all deferred event queues.
	void ClearDeferredQueues();

	/// Removes a callback handle from all callback vectors.
	void UnsubscribeFromAll(EventHandle Handle);

	// =========================================================================
	// Data Members
	// =========================================================================

	/// Platform backend for message translation.
	std::unique_ptr<IInputBackend> m_Backend;

	/// Pollable input state.
	InputState m_State;

	/// Mutex for callback list modifications (subscription/unsubscription).
	std::mutex m_CallbackMutex;

	/// Callback storage (tuple-based)
	CallbackTuple m_Callbacks;

	/// Deferred event queues (tuple-based)
	DeferredQueueTuple m_DeferredQueues;

	/// Next callback handle ID.
	uint32_t m_NextCallbackId = 1;

	/// Layer enable state (all enabled by default except Console/Debug).
	static constexpr std::size_t LayerCount = static_cast<std::size_t>(InputLayer::Count);
	std::array<bool, LayerCount> m_LayerEnabled = {true, false, false, true, true};
	// System=true, Console=false, Debug=false, HUD=true, Gameplay=true

	/// Previous mouse position for delta calculation.
	int32_t m_LastMouseX = 0;
	int32_t m_LastMouseY = 0;
	bool m_bHasLastMousePosition = false;

	/// Window message subscription (auto-cleanup via ScopedEventHandle).
	ScopedEventHandle m_windowMessageHandle;
};

// ============================================================================
// Template Implementations
// ============================================================================

template <typename TEvent> std::vector<InputSystem::CallbackEntry<TEvent>>& InputSystem::GetCallbacks()
{
	return std::get<std::vector<CallbackEntry<TEvent>>>(m_Callbacks);
}

template <typename TEvent> const std::vector<InputSystem::CallbackEntry<TEvent>>& InputSystem::GetCallbacks() const
{
	return std::get<std::vector<CallbackEntry<TEvent>>>(m_Callbacks);
}

template <typename TEvent> std::vector<TEvent>& InputSystem::GetDeferredQueue()
{
	return std::get<std::vector<TEvent>>(m_DeferredQueues);
}

template <typename TEvent> void InputSystem::DispatchToCallbacks(const TEvent& Event, DispatchMode TargetMode)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

	for (const auto& entry : GetCallbacks<TEvent>())
	{
		if (entry.Mode != TargetMode)
		{
			continue;
		}

		if (!ShouldDispatchToLayer(entry.Layer))
		{
			continue;
		}

		if (entry.Callback)
		{
			entry.Callback(Event);
		}
	}
}

template <typename TEvent> void InputSystem::QueueIfHasDeferredCallbacks(const TEvent& Event)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

	for (const auto& entry : GetCallbacks<TEvent>())
	{
		if (entry.Mode == DispatchMode::Deferred && ShouldDispatchToLayer(entry.Layer))
		{
			GetDeferredQueue<TEvent>().push_back(Event);
			return;
		}
	}
}

template <typename TEvent> void InputSystem::ProcessEvent(const TEvent& Event)
{
	UpdateStateFromEvent(Event);
	BroadcastToPublicEvent(Event);
	DispatchToCallbacks<TEvent>(Event, DispatchMode::Immediate);
	QueueIfHasDeferredCallbacks<TEvent>(Event);
}

template <typename TEvent> void InputSystem::ProcessDeferredEventsForType()
{
	auto& queue = GetDeferredQueue<TEvent>();
	for (const auto& event : queue)
	{
		DispatchToCallbacks<TEvent>(event, DispatchMode::Deferred);
	}
}
