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
#include "Core/Public/Threading/ThreadOwnership.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>

class InputEventDispatcher;
class Window;
struct WindowMessageEvent;

using InputEventTypes = std::tuple<KeyboardEvent, MouseButtonEvent, MouseMoveEvent, MouseWheelEvent>;

template <typename TEvent> using InputCallback = std::function<void(const TEvent&)>;

using KeyboardCallback = InputCallback<KeyboardEvent>;
using MouseButtonCallback = InputCallback<MouseButtonEvent>;
using MouseMoveCallback = InputCallback<MouseMoveEvent>;
using MouseWheelCallback = InputCallback<MouseWheelEvent>;
using InputCaptureQuery = std::function<bool()>;

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
	void SetInputCaptureQuery(InputCaptureQuery query) noexcept;
	void ClearInputCaptureQuery() noexcept;

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
	InputLayer ResolveTargetLayer(const InputBackendResult& Result);
	void CancelLayer(InputLayer Layer);

	template <typename TEvent> void ProcessEvent(const TEvent& Event, InputLayer TargetLayer);

	void UpdateStateFromEvent(InputState& State, const KeyboardEvent& Event);
	void UpdateStateFromEvent(InputState& State, const MouseButtonEvent& Event);
	void UpdateStateFromEvent(InputState& State, const MouseMoveEvent& Event);
	void UpdateStateFromEvent(InputState& State, const MouseWheelEvent& Event);

	std::unique_ptr<IInputBackend> m_Backend;
	std::unique_ptr<InputEventDispatcher> m_eventDispatcher;

	InputState m_State;
	std::array<InputState, LayerCount> m_LayerStates{};

	Threading::OwnerThread m_OwnerThread{"InputSystem registration and dispatch"};

	std::array<bool, LayerCount> m_LayerEnabled = {true, true, true};
	InputLayer m_ActiveLayer = InputLayer::Gameplay;
	InputLayer m_MouseCaptureLayer = InputLayer::System;
	InputFocusRouter m_FocusRouter;
	InputCaptureQuery m_captureQuery;

	int32_t m_LastMouseX = 0;
	int32_t m_LastMouseY = 0;
	bool m_bHasLastMousePosition = false;

	ScopedEventHandle m_windowMessageHandle;
};
