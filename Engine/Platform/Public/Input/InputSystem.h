#pragma once

#include "Platform/Public/PlatformAPI.h"

#include "IInputBackend.h"
#include "Events/EventHandle.h"
#include "Events/ScopedEventHandle.h"
#include "Input/InputState.h"
#include "Input/Dispatch/InputLayer.h"
#include "Input/Dispatch/DispatchMode.h"
#include "Input/Events/KeyboardEvent.h"
#include "Input/Events/MouseButtonEvent.h"
#include "Input/Events/MouseMoveEvent.h"
#include "Input/Events/MouseWheelEvent.h"
#include "Input/Routing/InputFocusRouter.h"

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
	const InputState& GetState(InputLayer Layer) const noexcept;

	void BeginInputRoutingFrame(bool interactionDisabled, bool textInputActive);
	void RegisterInputTargetRegion(float left, float top, float right, float bottom, InputLayer targetLayer);

	void SetActiveLayer(InputLayer Layer) noexcept;
	void SetLayerEnabled(InputLayer Layer, bool bEnabled);
	void SetAutomaticImGuiCaptureEnabled(bool enabled) noexcept;

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

	void CaptureMouse();

	void ReleaseMouse();

	bool IsMouseCaptured() const noexcept;

	void HideCursor();

	void ShowCursor();

	bool IsCursorHidden() const noexcept;

	void SetCursorVisibility(bool bVisible);

	void CenterCursor(void* windowHandle);

  private:
	static constexpr std::size_t LayerCount = static_cast<std::size_t>(InputLayer::Count);

	template <typename TEvent> struct CallbackEntry
	{
		std::function<void(const TEvent&)> Callback;
		EventHandle Handle;
		InputLayer Layer = InputLayer::Gameplay;
		DispatchMode Mode = DispatchMode::Immediate;
	};
	template <typename TEvent> struct RoutedInputEvent
	{
		TEvent Event;
		InputLayer TargetLayer = InputLayer::System;
	};

	using CallbackTuple = std::tuple<
	    std::vector<CallbackEntry<KeyboardEvent>>,
	    std::vector<CallbackEntry<MouseButtonEvent>>,
	    std::vector<CallbackEntry<MouseMoveEvent>>,
	    std::vector<CallbackEntry<MouseWheelEvent>>>;

	using DeferredQueueTuple = std::tuple<
	    std::vector<RoutedInputEvent<KeyboardEvent>>,
	    std::vector<RoutedInputEvent<MouseButtonEvent>>,
	    std::vector<RoutedInputEvent<MouseMoveEvent>>,
	    std::vector<RoutedInputEvent<MouseWheelEvent>>>;

	uint32_t GenerateCallbackId();

	bool ShouldDispatchToLayer(InputLayer RegisteredLayer, InputLayer TargetLayer) const noexcept;
	InputLayer ResolveTargetLayer(const InputBackendResult& Result);
	void CancelLayer(InputLayer Layer);

	template <typename TEvent> std::vector<CallbackEntry<TEvent>>& GetCallbacks();

	template <typename TEvent> const std::vector<CallbackEntry<TEvent>>& GetCallbacks() const;

	template <typename TEvent> std::vector<RoutedInputEvent<TEvent>>& GetDeferredQueue();

	template <typename TEvent> void DispatchToCallbacks(const TEvent& Event, DispatchMode TargetMode, InputLayer TargetLayer);

	template <typename TEvent> void QueueIfHasDeferredCallbacks(const TEvent& Event, InputLayer TargetLayer);

	template <typename TEvent> void ProcessEvent(const TEvent& Event, InputLayer TargetLayer);

	template <typename TEvent> void ProcessDeferredEventsForType();

	void UpdateStateFromEvent(InputState& State, const KeyboardEvent& Event);
	void UpdateStateFromEvent(InputState& State, const MouseButtonEvent& Event);
	void UpdateStateFromEvent(InputState& State, const MouseMoveEvent& Event);
	void UpdateStateFromEvent(InputState& State, const MouseWheelEvent& Event);

	void ClearDeferredQueues();

	void UnsubscribeFromAll(EventHandle Handle);

	std::unique_ptr<IInputBackend> m_Backend;

	InputState m_State;
	std::array<InputState, LayerCount> m_LayerStates{};

	std::mutex m_CallbackMutex;

	CallbackTuple m_Callbacks;

	DeferredQueueTuple m_DeferredQueues;

	uint32_t m_NextCallbackId = 1;

	std::array<bool, LayerCount> m_LayerEnabled = {true, true, true};
	InputLayer m_ActiveLayer = InputLayer::Gameplay;
	InputLayer m_MouseCaptureLayer = InputLayer::System;
	InputFocusRouter m_FocusRouter;
	bool m_automaticImGuiCaptureEnabled = true;

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

template <typename TEvent> std::vector<InputSystem::RoutedInputEvent<TEvent>>& InputSystem::GetDeferredQueue()
{
	return std::get<std::vector<RoutedInputEvent<TEvent>>>(m_DeferredQueues);
}

template <typename TEvent> void InputSystem::DispatchToCallbacks(const TEvent& Event, DispatchMode TargetMode, InputLayer TargetLayer)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

	for (const auto& entry : GetCallbacks<TEvent>())
	{
		if (entry.Mode != TargetMode)
		{
			continue;
		}

		if (!ShouldDispatchToLayer(entry.Layer, TargetLayer))
		{
			continue;
		}

		if (entry.Callback)
		{
			entry.Callback(Event);
		}
	}
}

template <typename TEvent> void InputSystem::QueueIfHasDeferredCallbacks(const TEvent& Event, InputLayer TargetLayer)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

	for (const auto& entry : GetCallbacks<TEvent>())
	{
		if (entry.Mode == DispatchMode::Deferred && ShouldDispatchToLayer(entry.Layer, TargetLayer))
		{
			GetDeferredQueue<TEvent>().push_back({Event, TargetLayer});
			return;
		}
	}
}

template <typename TEvent> void InputSystem::ProcessEvent(const TEvent& Event, InputLayer TargetLayer)
{
	UpdateStateFromEvent(m_State, Event);
	if (ShouldDispatchToLayer(TargetLayer, TargetLayer))
	{
		UpdateStateFromEvent(m_LayerStates[static_cast<std::size_t>(TargetLayer)], Event);
	}
	DispatchToCallbacks<TEvent>(Event, DispatchMode::Immediate, TargetLayer);
	QueueIfHasDeferredCallbacks<TEvent>(Event, TargetLayer);
}

template <typename TEvent> void InputSystem::ProcessDeferredEventsForType()
{
	auto& queue = GetDeferredQueue<TEvent>();
	for (const auto& routedEvent : queue)
	{
		DispatchToCallbacks<TEvent>(routedEvent.Event, DispatchMode::Deferred, routedEvent.TargetLayer);
	}
}
