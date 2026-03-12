// ============================================================================
// UI.h
// ----------------------------------------------------------------------------
// ImGui integration layer for Win32 and D3D12 backends.
//
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
	// ========================================================================
	// Lifecycle
	// ========================================================================

	UI(Timer& timer, D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager, D3D12SwapChain& swapChain);

	~UI() noexcept;

	UI(const UI&) = delete;
	UI& operator=(const UI&) = delete;
	UI(UI&&) = delete;
	UI& operator=(UI&&) = delete;

	// ========================================================================
	// Message Handling
	// ========================================================================

	void HandleWindowMessage(WindowMessageEvent& event) noexcept;

	/// @return True if ImGui consumed the message and app should skip it.
	bool ProcessWindowMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	// ========================================================================
	// Frame Operations
	// ========================================================================

	void Update();

	void Render() noexcept;

	// ========================================================================
	// Accessors
	// ========================================================================

	ViewMode::Type GetViewMode() noexcept;

  private:
	// ------------------------------------------------------------------------
	// Frame Building
	// ------------------------------------------------------------------------

	void NewFrame();

	void Build();

	// ------------------------------------------------------------------------
	// Initialization Helpers
	// ------------------------------------------------------------------------

	void InitializeImGuiContext();

	/// @return True if successful, false on failure.
	bool InitializeWin32Backend();

	/// @return True if successful, false on failure.
	bool InitializeD3D12Backend();

	/// Registers default UI panels and sections.
	void InitializeDefaultPanels();

	/// Subscribes to window message events for input handling.
	void SubscribeToWindowEvents(Window& window);

	void SetupDPIScaling() noexcept;

	// ------------------------------------------------------------------------
	// Owned Panels
	// ------------------------------------------------------------------------

	std::unique_ptr<RendererPanel> m_rendererPanel;                 ///< Settings panel for renderer options
	Timer* m_timer = nullptr;                                       ///< Timer reference for frame timing
	D3D12Rhi* m_rhi = nullptr;                                      ///< RHI reference for D3D12 operations
	Window* m_window = nullptr;                                     ///< Window reference for dimensions
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;  ///< Descriptor heap manager reference
	D3D12SwapChain* m_swapChain = nullptr;                          ///< Swap chain reference for back buffer format

	/// Window message subscription (auto-cleanup via ScopedEventHandle).
	ScopedEventHandle m_windowMessageHandle;
};
