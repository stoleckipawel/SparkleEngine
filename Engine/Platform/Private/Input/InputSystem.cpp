#include "PCH.h"
#include "InputSystem.h"
#include "Win32InputBackend.h"
#include "Window.h"

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <imgui.h>
#include <algorithm>
#include <cstdio>

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

void InputSystem::BeginFrame()
{
	m_State.BeginFrame();
	ClearDeferredQueues();
}

void InputSystem::EndFrame()
{
	m_State.EndFrame();
}

void InputSystem::ProcessDeferredEvents()
{
	ImGuiIO& io = ImGui::GetIO();
	SetLayerEnabled(InputLayer::Gameplay, !io.WantCaptureKeyboard && !io.WantCaptureMouse);

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
	}
}

bool InputSystem::OnWindowMessage(uint32_t Msg, uintptr_t Param1, intptr_t Param2)
{
	if (!m_Backend)
	{
		return false;
	}

	InputBackendResult result = m_Backend->ProcessMessage(Msg, Param1, Param2);

	if (!result.IsValid())
	{
		return false;
	}

	switch (result.Type)
	{
		case InputEventType::Keyboard:
			ProcessEvent(result.Keyboard);
			break;
		case InputEventType::MouseButton:
			ProcessEvent(result.MouseButton);
			break;
		case InputEventType::MouseMove:
			ProcessEvent(result.MouseMove);
			break;
		case InputEventType::MouseWheel:
			ProcessEvent(result.MouseWheel);
			break;
		default:
			return false;
	}

	return true;
}

void InputSystem::SetLayerEnabled(InputLayer Layer, bool bEnabled)
{
	const auto index = static_cast<std::size_t>(Layer);
	if (index < LayerCount)
	{
		m_LayerEnabled[index] = bEnabled;
	}
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
	for (std::size_t i = LayerCount; i > 0; --i)
	{
		const std::size_t index = i - 1;
		if (m_LayerEnabled[index])
		{
			return static_cast<InputLayer>(index);
		}
	}
	return InputLayer::Gameplay;
}

EventHandle InputSystem::SubscribeKeyboard(KeyboardCallback Callback, InputLayer Layer, DispatchMode Mode)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

	EventHandle handle{GenerateCallbackId()};
	GetCallbacks<KeyboardEvent>().push_back({std::move(Callback), handle, Layer, Mode});
	return handle;
}

EventHandle InputSystem::SubscribeMouseButton(MouseButtonCallback Callback, InputLayer Layer, DispatchMode Mode)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

	EventHandle handle{GenerateCallbackId()};
	GetCallbacks<MouseButtonEvent>().push_back({std::move(Callback), handle, Layer, Mode});
	return handle;
}

EventHandle InputSystem::SubscribeMouseMove(MouseMoveCallback Callback, InputLayer Layer, DispatchMode Mode)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

	EventHandle handle{GenerateCallbackId()};
	GetCallbacks<MouseMoveEvent>().push_back({std::move(Callback), handle, Layer, Mode});
	return handle;
}

EventHandle InputSystem::SubscribeMouseWheel(MouseWheelCallback Callback, InputLayer Layer, DispatchMode Mode)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

	EventHandle handle{GenerateCallbackId()};
	GetCallbacks<MouseWheelEvent>().push_back({std::move(Callback), handle, Layer, Mode});
	return handle;
}

void InputSystem::Unsubscribe(EventHandle Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	UnsubscribeFromAll(Handle);
}

void InputSystem::UnsubscribeFromAll(EventHandle Handle)
{
	std::lock_guard<std::mutex> lock(m_CallbackMutex);

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
	}
}

void InputSystem::ReleaseMouse()
{
	ReleaseCapture();
	m_State.SetMouseCaptured(false);
}

bool InputSystem::IsMouseCaptured() const noexcept
{
	return m_State.IsMouseCaptured();
}

void InputSystem::HideCursor()
{
	while (::ShowCursor(FALSE) >= 0)
	{
	}
	m_State.SetCursorHidden(true);
}

void InputSystem::ShowCursor()
{
	while (::ShowCursor(TRUE) < 0)
	{
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

bool InputSystem::ShouldDispatchToLayer(InputLayer Layer) const noexcept
{
	if (Layer == InputLayer::System)
	{
		return true;
	}

	if (!IsLayerEnabled(Layer))
	{
		return false;
	}

	InputLayer activeLayer = GetActiveLayer();
	return static_cast<uint8_t>(Layer) <= static_cast<uint8_t>(activeLayer);
}

void InputSystem::UpdateStateFromEvent(const KeyboardEvent& Event)
{
	m_State.SetKeyState(Event.KeyCode, Event.bPressed ? ButtonState::Pressed : ButtonState::Released);
	m_State.SetModifiers(Event.Modifiers);
}

void InputSystem::UpdateStateFromEvent(const MouseButtonEvent& Event)
{
	m_State.SetMouseButtonState(Event.Button, Event.bPressed ? ButtonState::Pressed : ButtonState::Released);
	m_State.SetMousePosition(Event.Position.X, Event.Position.Y);
	m_State.SetModifiers(Event.Modifiers);
}

void InputSystem::UpdateStateFromEvent(const MouseMoveEvent& Event)
{
	int32_t deltaX = 0;
	int32_t deltaY = 0;

	if (m_bHasLastMousePosition)
	{
		deltaX = Event.Position.X - m_LastMouseX;
		deltaY = Event.Position.Y - m_LastMouseY;
	}

	m_LastMouseX = Event.Position.X;
	m_LastMouseY = Event.Position.Y;
	m_bHasLastMousePosition = true;

	m_State.SetMousePosition(Event.Position.X, Event.Position.Y);
	m_State.AccumulateMouseDelta(deltaX, deltaY);
	m_State.SetModifiers(Event.Modifiers);
}

void InputSystem::UpdateStateFromEvent(const MouseWheelEvent& Event)
{
	if (Event.bHorizontal)
	{
		m_State.AccumulateWheelHorizontalDelta(Event.Delta);
	}
	else
	{
		m_State.AccumulateWheelDelta(Event.Delta);
	}

	m_State.SetMousePosition(Event.Position.X, Event.Position.Y);
}

void InputSystem::BroadcastToPublicEvent(const KeyboardEvent& Event)
{
	if (Event.bPressed)
	{
		OnKeyPressed.Broadcast(Event);
	}
	else
	{
		OnKeyReleased.Broadcast(Event);
	}
}

void InputSystem::BroadcastToPublicEvent(const MouseButtonEvent& Event)
{
	if (Event.bPressed)
	{
		OnMouseButtonPressed.Broadcast(Event);
	}
	else
	{
		OnMouseButtonReleased.Broadcast(Event);
	}
}

void InputSystem::BroadcastToPublicEvent(const MouseMoveEvent& Event)
{
	OnMouseMove.Broadcast(Event);
}

void InputSystem::BroadcastToPublicEvent(const MouseWheelEvent& Event)
{
	OnMouseWheel.Broadcast(Event);
}
