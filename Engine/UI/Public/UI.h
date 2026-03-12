#pragma once

#include "UI/Public/UIAPI.h"

#include "Events/ScopedEventHandle.h"

#include <Windows.h>

#include <memory>
#include "Sections/ViewMode.h"

class Timer;
class RendererPanel;
class UIRendererSection;
class StatsOverlay;
class Window;
class D3D12DescriptorHeapManager;
class D3D12SwapChain;
class D3D12Rhi;
struct WindowMessageEvent;

class SPARKLE_UI_API UI final
{
  public:
	UI(Timer& timer, D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager, D3D12SwapChain& swapChain);

	~UI() noexcept;

	UI(const UI&) = delete;
	UI& operator=(const UI&) = delete;
	UI(UI&&) = delete;
	UI& operator=(UI&&) = delete;

	void HandleWindowMessage(WindowMessageEvent& event) noexcept;

	bool ProcessWindowMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void Update();

	void Render() noexcept;

	ViewMode::Type GetViewMode() noexcept;

  private:
	void NewFrame();

	void Build();

	void InitializeImGuiContext();

	bool InitializeWin32Backend();

	bool InitializeD3D12Backend();

	void InitializeDefaultPanels();

	void SubscribeToWindowEvents(Window& window);

	void SetupDPIScaling() noexcept;

	std::unique_ptr<RendererPanel> m_rendererPanel;
	Timer* m_timer = nullptr;
	D3D12Rhi* m_rhi = nullptr;
	Window* m_window = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;

	ScopedEventHandle m_windowMessageHandle;
};
