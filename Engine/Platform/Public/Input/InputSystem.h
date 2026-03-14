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

class Window;
struct WindowMessageEvent;

using InputEventTypes = std::tuple<KeyboardEvent, MouseButtonEvent, MouseMoveEvent, MouseWheelEvent>;

template <typename TEvent> using InputCallback = std::function<void(const TEvent&)>;

using KeyboardCallback = InputCallback<KeyboardEvent>;
using MouseButtonCallback = InputCallback<MouseButtonEvent>;
using MouseMoveCallback = InputCallback<MouseMoveEvent>;
using MouseWheelCallback = InputCallback<MouseWheelEvent>;

class SPARKLE_PLATFORM_API InputSystem
{
  public:
	static std::unique_ptr<InputSystem> Create();

	explicit InputSystem(std::unique_ptr<IInputBackend> Backend);

	~InputSystem();

	InputSystem(const InputSystem&) = delete;
	InputSystem& operator=(const InputSystem&) = delete;

	InputSystem(InputSystem&&) = delete;
	InputSystem& operator=(InputSystem&&) = delete;

	void BeginFrame();

	void EndFrame();

	void ProcessDeferredEvents();

	void SubscribeToWindow(Window& window);

	void HandleWindowMessage(WindowMessageEvent& event);

	bool OnWindowMessage(uint32_t Msg, uintptr_t Param1, intptr_t Param2);

	const InputState& GetState() const noexcept { return m_State; }

	void SetLayerEnabled(InputLayer Layer, bool bEnabled);

	bool IsLayerEnabled(InputLayer Layer) const noexcept;

	InputLayer GetActiveLayer() const noexcept;

	EventHandle SubscribeKeyboard(
	    KeyboardCallback Callback,
	    InputLayer Layer = InputLayer::Gameplay,
	    DispatchMode Mode = DispatchMode::Immediate);

	EventHandle SubscribeMouseButton(
	    MouseButtonCallback Callback,
	    InputLayer Layer = InputLayer::Gameplay,
	    DispatchMode Mode = DispatchMode::Immediate);

	EventHandle SubscribeMouseMove(
	    MouseMoveCallback Callback,
	    InputLayer Layer = InputLayer::Gameplay,
	    DispatchMode Mode = DispatchMode::Immediate);

	EventHandle SubscribeMouseWheel(
	    MouseWheelCallback Callback,
	    InputLayer Layer = InputLayer::Gameplay,
	    DispatchMode Mode = DispatchMode::Immediate);

	void Unsubscribe(EventHandle Handle);

	Event<void(const KeyboardEvent&)> OnKeyPressed;

	Event<void(const KeyboardEvent&)> OnKeyReleased;

	Event<void(const MouseButtonEvent&)> OnMouseButtonPressed;

	Event<void(const MouseButtonEvent&)> OnMouseButtonReleased;

	Event<void(const MouseMoveEvent&)> OnMouseMove;

	Event<void(const MouseWheelEvent&)> OnMouseWheel;

	void CaptureMouse();

	void ReleaseMouse();

	bool IsMouseCaptured() const noexcept;

	void HideCursor();

	void ShowCursor();

	bool IsCursorHidden() const noexcept;

	void SetCursorVisibility(bool bVisible);

	void CenterCursor(void* windowHandle);

  private:
	template <typename TEvent> struct CallbackEntry
	{
		std::function<void(const TEvent&)> Callback;
		EventHandle Handle;
		InputLayer Layer = InputLayer::Gameplay;
		DispatchMode Mode = DispatchMode::Immediate;
	};

	using CallbackTuple = std::tuple<
	    std::vector<CallbackEntry<KeyboardEvent>>,
	    std::vector<CallbackEntry<MouseButtonEvent>>,
	    std::vector<CallbackEntry<MouseMoveEvent>>,
	    std::vector<CallbackEntry<MouseWheelEvent>>>;

	using DeferredQueueTuple =
	    std::tuple<std::vector<KeyboardEvent>, std::vector<MouseButtonEvent>, std::vector<MouseMoveEvent>, std::vector<MouseWheelEvent>>;

	uint32_t GenerateCallbackId();

	bool ShouldDispatchToLayer(InputLayer Layer) const noexcept;

	template <typename TEvent> std::vector<CallbackEntry<TEvent>>& GetCallbacks();

	template <typename TEvent> const std::vector<CallbackEntry<TEvent>>& GetCallbacks() const;

	template <typename TEvent> std::vector<TEvent>& GetDeferredQueue();

	template <typename TEvent> void DispatchToCallbacks(const TEvent& Event, DispatchMode TargetMode);

	template <typename TEvent> void QueueIfHasDeferredCallbacks(const TEvent& Event);

	template <typename TEvent> void ProcessEvent(const TEvent& Event);

	template <typename TEvent> void ProcessDeferredEventsForType();

	void UpdateStateFromEvent(const KeyboardEvent& Event);
	void UpdateStateFromEvent(const MouseButtonEvent& Event);
	void UpdateStateFromEvent(const MouseMoveEvent& Event);
	void UpdateStateFromEvent(const MouseWheelEvent& Event);

	void BroadcastToPublicEvent(const KeyboardEvent& Event);
	void BroadcastToPublicEvent(const MouseButtonEvent& Event);
	void BroadcastToPublicEvent(const MouseMoveEvent& Event);
	void BroadcastToPublicEvent(const MouseWheelEvent& Event);

	void ClearDeferredQueues();

	void UnsubscribeFromAll(EventHandle Handle);

	std::unique_ptr<IInputBackend> m_Backend;

	InputState m_State;

	std::mutex m_CallbackMutex;

	CallbackTuple m_Callbacks;

	DeferredQueueTuple m_DeferredQueues;

	uint32_t m_NextCallbackId = 1;

	static constexpr std::size_t LayerCount = static_cast<std::size_t>(InputLayer::Count);
	std::array<bool, LayerCount> m_LayerEnabled = {true, true};

	int32_t m_LastMouseX = 0;
	int32_t m_LastMouseY = 0;
	bool m_bHasLastMousePosition = false;

	ScopedEventHandle m_windowMessageHandle;
};

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
