#pragma once

#include "Platform/Public/PlatformAPI.h"
#include "Events/Event.h"
#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

struct SPARKLE_PLATFORM_API WindowMessageEvent
{
	HWND hWnd;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
	bool handled = false;
};

class SPARKLE_PLATFORM_API Window final
{
public:
	enum class State
	{
		Normal,
		Minimized,
		Maximized,
		FullScreen
	};

	explicit Window(std::string_view windowTitle);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	// PollEvents broadcasts window events synchronously on its calling thread.
	Event<void()> OnResized;
	Event<void(float)> OnDpiScaleChanged;

	Event<void(WindowMessageEvent&), 16> OnWindowMessage;

	void PollEvents() noexcept;
	void WaitForEvent() noexcept;

	HWND GetHWND() const noexcept { return m_hWnd; }

	uint32_t GetWidth() const noexcept { return m_clientWidth.load(std::memory_order_acquire); }
	uint32_t GetHeight() const noexcept { return m_clientHeight.load(std::memory_order_acquire); }
	float GetDpiScale() const noexcept;
	bool HasValidSize() const noexcept;

	State GetState() const noexcept { return m_state; }
	bool ShouldClose() const noexcept { return m_bShouldClose; }
	bool IsFullScreen() const noexcept { return m_state == State::FullScreen; }
	bool IsMinimized() const noexcept { return m_state == State::Minimized; }
	bool IsMaximized() const noexcept { return m_state == State::Maximized; }

	void SetFullScreen(bool bFullScreen);
	void ToggleFullScreen();
	void Minimize() noexcept;
	void Maximize() noexcept;
	void Restore() noexcept;
	void ToggleMaximizeRestore() noexcept;
	void BeginDragMove() noexcept;

	void RequestClose() noexcept { m_bShouldClose = true; }

private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void RegisterWindowClass();
	void CreateWindowHandle(std::string_view title);
	void ApplyInitialWindowState();
	static bool ShouldStartFullscreen() noexcept;
	UINT GetDpi() const noexcept;
	int GetResizeBorderThickness() const noexcept;
	MONITORINFO GetCurrentMonitorInfo() const noexcept;

	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void HandleDpiChanged(UINT dpi, const RECT& suggestedWindowRect);
	void OnSizeChanged(WPARAM sizeType, uint32_t width, uint32_t height);
	void ApplyPendingShowCommand() noexcept;

	static constexpr const wchar_t* kWindowClassName = L"SparkleEngineWindow";
	static constexpr DWORD kWindowedStyle = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
	static constexpr DWORD kWindowedExStyle = WS_EX_APPWINDOW;
	static constexpr LONG kMinWindowWidth = 320;
	static constexpr LONG kMinWindowHeight = 240;
	static constexpr int kNoShowCommand = -1;

	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;
	ATOM m_windowClassAtom = 0;

	RECT m_windowedRect{};
	std::atomic<uint32_t> m_clientWidth{0};
	std::atomic<uint32_t> m_clientHeight{0};
	State m_state = State::Normal;
	int m_pendingShowCommand = kNoShowCommand;
	bool m_bShouldClose = false;
};
