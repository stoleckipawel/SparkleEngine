// =============================================================================
// Window.h - Platform Window Management
// =============================================================================
//
// Manages the main application window, message handling, and display state.
// Encapsulates Win32 window creation and provides a clean interface for
// querying window dimensions and fullscreen state.
//
#pragma once

#include "Platform/Public/PlatformAPI.h"
#include "Events/Event.h"
#include <Windows.h>
#include <cstdint>
#include <string>
#include <string_view>

// =============================================================================
// Window Message Event Data
// =============================================================================

/// Data passed to window message subscribers.
struct SPARKLE_PLATFORM_API WindowMessageEvent
{
	HWND hWnd;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
	bool handled = false;  ///< Set to true if subscriber handled the message
};

// =============================================================================
// Window
// =============================================================================

class SPARKLE_PLATFORM_API Window final
{
  public:
	// =========================================================================
	// Lifecycle
	// =========================================================================

	explicit Window(std::string_view windowTitle);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	// =========================================================================
	// Events
	// =========================================================================

	/// Broadcast when window is resized. Subscribe to handle resize logic.
	Event<void()> OnResized;

	/// Broadcast for each window message. Subscribers can set event.handled = true
	/// to indicate the message was consumed. Higher priority handlers are called first.
	/// Return value indicates if message was handled.
	Event<void(WindowMessageEvent&), 16> OnWindowMessage;

	// =========================================================================
	// Frame Operations
	// =========================================================================

	/// Processes pending Win32 messages. Call once per frame.
	void PollEvents() noexcept;

	// =========================================================================
	// Accessors
	// =========================================================================

	HWND GetHWND() const noexcept { return m_hWnd; }

	uint32_t GetWidth() const noexcept;
	uint32_t GetHeight() const noexcept;

	bool ShouldClose() const noexcept { return m_bShouldClose; }
	bool IsFullScreen() const noexcept { return m_bIsFullScreen; }
	bool IsMinimized() const noexcept { return m_bIsMinimized; }

	// =========================================================================
	// State Modifiers
	// =========================================================================

	/// Toggles fullscreen mode (borderless windowed). Preserves windowed position.
	void SetFullScreen(bool bFullScreen);

	/// Requests the window to close. ShouldClose() will return true after this.
	void RequestClose() noexcept { m_bShouldClose = true; }

  private:
	// -------------------------------------------------------------------------
	// Win32 Internals
	// -------------------------------------------------------------------------

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void RegisterWindowClass();
	void CreateWindowHandle(std::string_view title);
	void ApplyInitialWindowState();

	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void OnSizeChanged(WPARAM sizeType, uint32_t width, uint32_t height);

	// -------------------------------------------------------------------------
	// Constants
	// -------------------------------------------------------------------------

	static constexpr const wchar_t* kWindowClassName = L"SparkleEngineWindow";

	// -------------------------------------------------------------------------
	// Win32 Handles
	// -------------------------------------------------------------------------

	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;
	ATOM m_windowClassAtom = 0;

	// -------------------------------------------------------------------------
	// Window State
	// -------------------------------------------------------------------------

	RECT m_windowedRect{};  ///< Saved window rect for fullscreen toggle restore
	bool m_bShouldClose = false;
	bool m_bIsFullScreen = false;
	bool m_bIsMinimized = false;
};