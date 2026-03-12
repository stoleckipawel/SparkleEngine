#include "PCH.h"
#include "Window.h"
#include "PlatformConfig.h"
#include "Diagnostics/Log.h"

Window::Window(std::string_view windowTitle)
{
	m_hInstance = GetModuleHandleW(nullptr);

	RegisterWindowClass();
	CreateWindowHandle(windowTitle);
	ApplyInitialWindowState();
}

Window::~Window()
{
	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}

	if (m_windowClassAtom && m_hInstance)
	{
		UnregisterClassW(kWindowClassName, m_hInstance);
		m_windowClassAtom = 0;
	}
}

void Window::RegisterWindowClass()
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = &Window::WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(Window*);
	wc.hInstance = m_hInstance;
	wc.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
	wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = kWindowClassName;
	wc.hIconSm = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));

	m_windowClassAtom = RegisterClassExW(&wc);
	if (!m_windowClassAtom)
	{
		LOG_FATAL("Window: Failed to register window class");
	}
}

void Window::CreateWindowHandle(std::string_view title)
{
	constexpr DWORD kWindowStyle = WS_OVERLAPPEDWINDOW;
	constexpr DWORD kWindowExStyle = WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW;

	const std::wstring wideTitle(title.begin(), title.end());

	m_hWnd = CreateWindowExW(
	    kWindowExStyle,
	    kWindowClassName,
	    wideTitle.c_str(),
	    kWindowStyle,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    nullptr,
	    nullptr,
	    m_hInstance,
	    this
	);

	if (!m_hWnd)
	{
		LOG_FATAL("Window: Failed to create window");
	}
}

void Window::ApplyInitialWindowState()
{
	GetWindowRect(m_hWnd, &m_windowedRect);

	if (PlatformSettings::StartFullscreen)
	{
		SetFullScreen(true);
	}
	else
	{
		ShowWindow(m_hWnd, SW_SHOWMAXIMIZED);
	}
}

void Window::PollEvents() noexcept
{
	MSG msg{};
	while (PeekMessageW(&msg, m_hWnd, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

uint32_t Window::GetWidth() const noexcept
{
	RECT rect{};
	if (m_hWnd && GetClientRect(m_hWnd, &rect))
	{
		return static_cast<uint32_t>(rect.right - rect.left);
	}
	return 0;
}

uint32_t Window::GetHeight() const noexcept
{
	RECT rect{};
	if (m_hWnd && GetClientRect(m_hWnd, &rect))
	{
		return static_cast<uint32_t>(rect.bottom - rect.top);
	}
	return 0;
}

void Window::SetFullScreen(bool bFullScreen)
{
	if (m_bIsFullScreen == bFullScreen)
	{
		return;
	}

	if (bFullScreen)
	{
		GetWindowRect(m_hWnd, &m_windowedRect);

		SetWindowLongW(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowLongW(m_hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW);

		HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo{};
		monitorInfo.cbSize = sizeof(MONITORINFO);

		if (GetMonitorInfoW(hMonitor, &monitorInfo))
		{
			const RECT& rc = monitorInfo.rcMonitor;
			SetWindowPos(m_hWnd, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
		}

		ShowWindow(m_hWnd, SW_SHOW);
	}
	else
	{
		SetWindowLongW(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowLongW(m_hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW);

		SetWindowPos(
		    m_hWnd,
		    HWND_NOTOPMOST,
		    m_windowedRect.left,
		    m_windowedRect.top,
		    m_windowedRect.right - m_windowedRect.left,
		    m_windowedRect.bottom - m_windowedRect.top,
		    SWP_FRAMECHANGED | SWP_NOACTIVATE);

		ShowWindow(m_hWnd, SW_SHOWMAXIMIZED);
	}

	m_bIsFullScreen = bFullScreen;
}

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Window* window = nullptr;

	if (msg == WM_NCCREATE)
	{
		auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
		window = static_cast<Window*>(create->lpCreateParams);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

		window->m_hWnd = hWnd;
	}
	else
	{
		window = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	}

	if (window)
	{
		return window->HandleMessage(msg, wParam, lParam);
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	WindowMessageEvent msgEvent{m_hWnd, msg, wParam, lParam, false};
	OnWindowMessage.Broadcast(msgEvent);

	if (msgEvent.handled)
	{
		return 0;
	}

	switch (msg)
	{
		case WM_SIZE:
			OnSizeChanged(wParam, LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_CLOSE:
			m_bShouldClose = true;
			return 0;

		case WM_DESTROY:
			m_hWnd = nullptr;
			return 0;

		case WM_KEYDOWN:

			if (wParam == VK_F11)
			{
				SetFullScreen(!m_bIsFullScreen);
				return 0;
			}
			break;

		case WM_SYSKEYDOWN:

			if (wParam == VK_RETURN && (lParam & (1 << 29)))
			{
				SetFullScreen(!m_bIsFullScreen);
				return 0;
			}
			break;

		case WM_GETMINMAXINFO:
		{
			auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
			minMaxInfo->ptMinTrackSize.x = 320;
			minMaxInfo->ptMinTrackSize.y = 240;
			return 0;
		}

		case WM_ACTIVATEAPP:

			break;

		case WM_ERASEBKGND:

			return 1;
	}

	return DefWindowProcW(m_hWnd, msg, wParam, lParam);
}

void Window::OnSizeChanged(WPARAM sizeType, [[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
{
	switch (sizeType)
	{
		case SIZE_MINIMIZED:
			m_bIsMinimized = true;
			break;

		case SIZE_RESTORED:
		case SIZE_MAXIMIZED:
			m_bIsMinimized = false;
			OnResized.Broadcast();
			break;
	}
}
