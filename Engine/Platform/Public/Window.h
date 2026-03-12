#pragma once

#include "Platform/Public/PlatformAPI.h"
#include "Events/Event.h"
#include <Windows.h>
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
	explicit Window(std::string_view windowTitle);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	Event<void()> OnResized;

	Event<void(WindowMessageEvent&), 16> OnWindowMessage;

	void PollEvents() noexcept;

	HWND GetHWND() const noexcept { return m_hWnd; }

	uint32_t GetWidth() const noexcept;
	uint32_t GetHeight() const noexcept;

	bool ShouldClose() const noexcept { return m_bShouldClose; }
	bool IsFullScreen() const noexcept { return m_bIsFullScreen; }
	bool IsMinimized() const noexcept { return m_bIsMinimized; }

	void SetFullScreen(bool bFullScreen);

	void RequestClose() noexcept { m_bShouldClose = true; }

  private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void RegisterWindowClass();
	void CreateWindowHandle(std::string_view title);
	void ApplyInitialWindowState();

	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void OnSizeChanged(WPARAM sizeType, uint32_t width, uint32_t height);

	static constexpr const wchar_t* kWindowClassName = L"SparkleEngineWindow";

	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;
	ATOM m_windowClassAtom = 0;

	RECT m_windowedRect{};
	bool m_bShouldClose = false;
	bool m_bIsFullScreen = false;
	bool m_bIsMinimized = false;
};