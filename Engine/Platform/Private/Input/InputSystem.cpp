#include "PCH.h"
#include "Input/InputSystem.h"
#include "Input/Backends/Win32InputBackend.h"
#include "Window/Window.h"

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <algorithm>
#include <cstdio>
#include <utility>

class InputDeferredEventProcessingGuard final
{
  public:
	explicit InputDeferredEventProcessingGuard(bool& isProcessing) noexcept : m_isProcessing(isProcessing)
	{
		if (!m_isProcessing)
		{
			m_isProcessing = true;
			m_didBegin = true;
		}
	}

	~InputDeferredEventProcessingGuard()
	{
		if (m_didBegin)
		{
			m_isProcessing = false;
		}
	}

	InputDeferredEventProcessingGuard(const InputDeferredEventProcessingGuard&) = delete;
	InputDeferredEventProcessingGuard& operator=(const InputDeferredEventProcessingGuard&) = delete;

	explicit operator bool() const noexcept { return m_didBegin; }

  private:
	bool& m_isProcessing;
	bool m_didBegin = false;
};

std::unique_ptr<InputSystem> InputSystem::Create()
{
#if defined(_WIN32) || defined(_WIN64)
	auto backend = std::make_unique<Win32InputBackend>();
	return std::make_unique<InputSystem>(std::move(backend));
#else
	#error "No input backend available for this platform"
#endif
}

InputSystem::InputSystem(std::unique_ptr<IInputBackend> Backend) : m_Backend(std::move(Backend)) {}

InputSystem::~InputSystem() = default;

const InputState& InputSystem::GetState(InputLayer Layer) const noexcept
{
	if (Layer == InputLayer::System)
	{
		return m_State;
	}

	const auto index = static_cast<std::size_t>(Layer);
	return index < LayerCount ? m_LayerStates[index] : m_State;
}

void InputSystem::BeginFrame()
{
	m_OwnerThread.AssertAccess();
	m_State.BeginFrame();
	for (InputState& layerState : m_LayerStates)
	{
		layerState.BeginFrame();
	}
	ClearDeferredQueues();
}

void InputSystem::ProcessDeferredEvents()
{
	m_OwnerThread.AssertAccess();
	const InputDeferredEventProcessingGuard processing(m_bIsProcessingDeferredEvents);
	if (!processing)
	{
		return;
	}

	if (m_captureQuery)
	{
		const bool wantsCaptureInput = m_captureQuery();
		SetActiveLayer(wantsCaptureInput ? InputLayer::UI : InputLayer::Gameplay);
	}

	ProcessDeferredEventsForType<KeyboardEvent>();
	ProcessDeferredEventsForType<MouseButtonEvent>();
	ProcessDeferredEventsForType<MouseMoveEvent>();
	ProcessDeferredEventsForType<MouseWheelEvent>();
}

void InputSystem::ClearDeferredQueues()
{
	GetDeferredQueue<KeyboardEvent>().clear();
	GetDeferredQueue<MouseButtonEvent>().clear();
	GetDeferredQueue<MouseMoveEvent>().clear();
	GetDeferredQueue<MouseWheelEvent>().clear();
}

void InputSystem::SubscribeToWindow(Window& window)
{
	m_OwnerThread.AssertAccess();
	auto handle = window.OnWindowMessage.Add(
	    [this](WindowMessageEvent& event)
	    {
		    HandleWindowMessage(event);
	    });
	m_windowMessageHandle = ScopedEventHandle(window.OnWindowMessage, handle);
}

void InputSystem::HandleWindowMessage(WindowMessageEvent& event)
{
	if (OnWindowMessage(event.msg, event.wParam, event.lParam))
	{
		event.handled = true;
	}
}

bool InputSystem::OnWindowMessage(uint32_t Msg, uintptr_t Param1, intptr_t Param2)
{
	m_OwnerThread.AssertAccess();
	if (!m_Backend)
	{
		return false;
	}

	InputBackendResult result = m_Backend->ProcessMessage(Msg, Param1, Param2);

	if (!result.IsValid())
	{
		return false;
	}
	if (result.Type == InputEventType::MouseMove)
	{
		int32_t deltaX = 0;
		int32_t deltaY = 0;

		if (m_bHasLastMousePosition)
		{
			deltaX = result.MouseMove.Position.X - m_LastMouseX;
			deltaY = result.MouseMove.Position.Y - m_LastMouseY;
		}

		m_LastMouseX = result.MouseMove.Position.X;
		m_LastMouseY = result.MouseMove.Position.Y;
		m_bHasLastMousePosition = true;
		result.MouseMove.Delta.X = deltaX;
		result.MouseMove.Delta.Y = deltaY;
	}

	const InputLayer targetLayer = ResolveTargetLayer(result);
	if (result.Type == InputEventType::MouseButton && result.MouseButton.bPressed && targetLayer != InputLayer::System)
	{
		SetActiveLayer(targetLayer);
	}

	switch (result.Type)
	{
		case InputEventType::Keyboard:
			ProcessEvent(result.Keyboard, targetLayer);
			break;
		case InputEventType::MouseButton:
			ProcessEvent(result.MouseButton, targetLayer);
			break;
		case InputEventType::MouseMove:
			ProcessEvent(result.MouseMove, targetLayer);
			break;
		case InputEventType::MouseWheel:
			ProcessEvent(result.MouseWheel, targetLayer);
			break;
		default:
			return false;
	}

	return true;
}

void InputSystem::BeginInputRoutingFrame(bool interactionDisabled, bool textInputActive)
{
	m_FocusRouter.BeginFrame(interactionDisabled, textInputActive);
}

void InputSystem::RegisterInputTargetRegion(float left, float top, float right, float bottom, InputLayer targetLayer)
{
	m_FocusRouter.RegisterTargetRegion(left, top, right, bottom, targetLayer);
}

void InputSystem::SetActiveLayer(InputLayer Layer) noexcept
{
	if (!IsLayerEnabled(Layer))
	{
		Layer = InputLayer::System;
	}

	if (m_ActiveLayer == Layer)
	{
		return;
	}

	const InputLayer oldLayer = m_ActiveLayer;
	m_ActiveLayer = Layer;
	if (oldLayer != InputLayer::System)
	{
		CancelLayer(oldLayer);
	}
}

void InputSystem::SetLayerEnabled(InputLayer Layer, bool bEnabled)
{
	const auto index = static_cast<std::size_t>(Layer);
	if (index < LayerCount)
	{
		m_LayerEnabled[index] = bEnabled;
		if (!bEnabled && m_ActiveLayer == Layer)
		{
			SetActiveLayer(InputLayer::System);
		}
	}
}

void InputSystem::SetInputCaptureQuery(InputCaptureQuery query) noexcept
{
	m_captureQuery = std::move(query);
}

void InputSystem::ClearInputCaptureQuery() noexcept
{
	m_captureQuery = {};
}

bool InputSystem::IsLayerEnabled(InputLayer Layer) const noexcept
{
	const auto index = static_cast<std::size_t>(Layer);
	if (index < LayerCount)
	{
		return m_LayerEnabled[index];
	}
	return false;
}

InputLayer InputSystem::GetActiveLayer() const noexcept
{
	return IsLayerEnabled(m_ActiveLayer) ? m_ActiveLayer : InputLayer::System;
}

InputLayer InputSystem::ResolveTargetLayer(const InputBackendResult& Result)
{
	if (m_State.IsMouseCaptured() && IsLayerEnabled(m_MouseCaptureLayer))
	{
		return m_MouseCaptureLayer;
	}

	InputLayer targetLayer = m_FocusRouter.ResolveTargetLayer(Result, GetActiveLayer());

	return IsLayerEnabled(targetLayer) ? targetLayer : InputLayer::System;
}

EventHandle InputSystem::SubscribeKeyboard(KeyboardCallback Callback, InputLayer Layer, DispatchMode Mode)
{
	m_OwnerThread.AssertAccess();
	EventHandle handle{GenerateCallbackId()};
	GetCallbacks<KeyboardEvent>().push_back({std::move(Callback), handle, Layer, Mode});
	return handle;
}

EventHandle InputSystem::SubscribeMouseButton(MouseButtonCallback Callback, InputLayer Layer, DispatchMode Mode)
{
	m_OwnerThread.AssertAccess();
	EventHandle handle{GenerateCallbackId()};
	GetCallbacks<MouseButtonEvent>().push_back({std::move(Callback), handle, Layer, Mode});
	return handle;
}

EventHandle InputSystem::SubscribeMouseMove(MouseMoveCallback Callback, InputLayer Layer, DispatchMode Mode)
{
	m_OwnerThread.AssertAccess();
	EventHandle handle{GenerateCallbackId()};
	GetCallbacks<MouseMoveEvent>().push_back({std::move(Callback), handle, Layer, Mode});
	return handle;
}

EventHandle InputSystem::SubscribeMouseWheel(MouseWheelCallback Callback, InputLayer Layer, DispatchMode Mode)
{
	m_OwnerThread.AssertAccess();
	EventHandle handle{GenerateCallbackId()};
	GetCallbacks<MouseWheelEvent>().push_back({std::move(Callback), handle, Layer, Mode});
	return handle;
}

void InputSystem::Unsubscribe(EventHandle Handle)
{
	m_OwnerThread.AssertAccess();
	if (!Handle.IsValid())
	{
		return;
	}

	UnsubscribeFromAll(Handle);
}

void InputSystem::UnsubscribeFromAll(EventHandle Handle)
{
	auto removeByHandle = [&Handle](auto& callbacks)
	{
		callbacks.erase(
		    std::remove_if(
		        callbacks.begin(),
		        callbacks.end(),
		        [&Handle](const auto& entry)
		        {
			        return entry.Handle == Handle;
		        }),
		    callbacks.end());
	};

	std::apply(
	    [&removeByHandle](auto&... callbacks)
	    {
		    (removeByHandle(callbacks), ...);
	    },
	    m_Callbacks);
}

void InputSystem::CaptureMouse()
{
	HWND hWnd = GetForegroundWindow();
	if (hWnd)
	{
		SetCapture(hWnd);
		m_State.SetMouseCaptured(true);
		m_MouseCaptureLayer = GetActiveLayer();
	}
}

void InputSystem::ReleaseMouse()
{
	ReleaseCapture();
	m_State.SetMouseCaptured(false);
	m_MouseCaptureLayer = InputLayer::System;
}

bool InputSystem::IsMouseCaptured() const noexcept
{
	return m_State.IsMouseCaptured();
}

void InputSystem::HideCursor()
{
	while (::ShowCursor(FALSE) >= 0)
	{
		continue;
	}
	m_State.SetCursorHidden(true);
}

void InputSystem::ShowCursor()
{
	while (::ShowCursor(TRUE) < 0)
	{
		continue;
	}
	m_State.SetCursorHidden(false);
}

bool InputSystem::IsCursorHidden() const noexcept
{
	return m_State.IsCursorHidden();
}

void InputSystem::CenterCursor(void* windowHandle)
{
	HWND hWnd = static_cast<HWND>(windowHandle);
	if (!hWnd)
	{
		return;
	}

	RECT rect;
	if (GetClientRect(hWnd, &rect))
	{
		POINT center;
		center.x = (rect.right - rect.left) / 2;
		center.y = (rect.bottom - rect.top) / 2;
		ClientToScreen(hWnd, &center);
		SetCursorPos(center.x, center.y);

		ScreenToClient(hWnd, &center);
		m_LastMouseX = center.x;
		m_LastMouseY = center.y;
	}
}

void InputSystem::SetCursorVisibility(bool bVisible)
{
	const bool bCurrentlyHidden = m_State.IsCursorHidden();
	if (bVisible && !bCurrentlyHidden)
	{
		return;
	}
	if (!bVisible && bCurrentlyHidden)
	{
		return;
	}

	if (bVisible)
	{
		ShowCursor();
	}
	else
	{
		HideCursor();
	}
}

uint32_t InputSystem::GenerateCallbackId()
{
	return m_NextCallbackId++;
}

bool InputSystem::ShouldDispatchToLayer(InputLayer RegisteredLayer, InputLayer TargetLayer) const noexcept
{
	if (RegisteredLayer == InputLayer::System)
	{
		return true;
	}

	if (RegisteredLayer != TargetLayer)
	{
		return false;
	}

	return IsLayerEnabled(RegisteredLayer);
}

void InputSystem::CancelLayer(InputLayer Layer)
{
	if (Layer == InputLayer::System)
	{
		return;
	}

	const auto layerIndex = static_cast<std::size_t>(Layer);
	if (layerIndex >= LayerCount)
	{
		return;
	}

	InputState& state = m_LayerStates[layerIndex];
	for (std::size_t keyIndex = 0; keyIndex < InputState::KeyCount; ++keyIndex)
	{
		const ButtonState keyState = state.m_KeyStates[keyIndex];
		if (keyState != ButtonState::Pressed && keyState != ButtonState::Held)
		{
			continue;
		}

		KeyboardEvent releaseEvent{};
		releaseEvent.KeyCode = static_cast<Key>(keyIndex);
		releaseEvent.Modifiers = state.GetModifiers();
		releaseEvent.bPressed = false;
		state.SetKeyState(releaseEvent.KeyCode, ButtonState::Released);
		DispatchToCallbacks(releaseEvent, DispatchMode::Immediate, Layer);
	}

	const MousePosition mousePosition = state.GetMousePosition();
	for (std::size_t buttonIndex = 0; buttonIndex < InputState::MouseButtonCount; ++buttonIndex)
	{
		const ButtonState buttonState = state.m_MouseButtonStates[buttonIndex];
		if (buttonState != ButtonState::Pressed && buttonState != ButtonState::Held)
		{
			continue;
		}

		MouseButtonEvent releaseEvent{};
		releaseEvent.Button = static_cast<MouseButton>(buttonIndex);
		releaseEvent.Position = mousePosition;
		releaseEvent.Modifiers = state.GetModifiers();
		releaseEvent.bPressed = false;
		state.SetMouseButtonState(releaseEvent.Button, ButtonState::Released);
		DispatchToCallbacks(releaseEvent, DispatchMode::Immediate, Layer);
	}

	state = InputState{};
}

void InputSystem::UpdateStateFromEvent(InputState& State, const KeyboardEvent& Event)
{
	State.SetKeyState(Event.KeyCode, Event.bPressed ? ButtonState::Pressed : ButtonState::Released);
	State.SetModifiers(Event.Modifiers);
}

void InputSystem::UpdateStateFromEvent(InputState& State, const MouseButtonEvent& Event)
{
	State.SetMouseButtonState(Event.Button, Event.bPressed ? ButtonState::Pressed : ButtonState::Released);
	State.SetMousePosition(Event.Position.X, Event.Position.Y);
	State.SetModifiers(Event.Modifiers);
}

void InputSystem::UpdateStateFromEvent(InputState& State, const MouseMoveEvent& Event)
{
	State.SetMousePosition(Event.Position.X, Event.Position.Y);
	State.AccumulateMouseDelta(Event.Delta.X, Event.Delta.Y);
	State.SetModifiers(Event.Modifiers);
}

void InputSystem::UpdateStateFromEvent(InputState& State, const MouseWheelEvent& Event)
{
	if (Event.bHorizontal)
	{
		State.AccumulateWheelHorizontalDelta(Event.Delta);
	}
	else
	{
		State.AccumulateWheelDelta(Event.Delta);
	}

	State.SetMousePosition(Event.Position.X, Event.Position.Y);
}
