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

	/// Creates ImGui context and initializes Win32/DX12 backends.
	UI(Timer& timer, D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager, D3D12SwapChain& swapChain);

	/// Shuts down backends and destroys ImGui context.
	~UI() noexcept;

	UI(const UI&) = delete;
	UI& operator=(const UI&) = delete;
	UI(UI&&) = delete;
	UI& operator=(UI&&) = delete;

	// ========================================================================
	// Message Handling
	// ========================================================================

	/// Handles window message events from the Window's event system.
	/// @param event Window message event data
	void HandleWindowMessage(WindowMessageEvent& event) noexcept;

	/// Forwards Win32 messages to ImGui for input processing (internal use).
	/// @return True if ImGui consumed the message and app should skip it.
	bool ProcessWindowMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	// ========================================================================
	// Frame Operations
	// ========================================================================

	/// Updates UI state and builds draw lists for the current frame.
	void Update();

	/// Submits ImGui draw data to the current DX12 command list.
	void Render() noexcept;

	// ========================================================================
	// Accessors
	// ========================================================================

	/// Returns the current view mode selected in the UI.
	ViewMode::Type GetViewMode() noexcept;

  private:
	// ------------------------------------------------------------------------
	// Frame Building
	// ------------------------------------------------------------------------

	/// Begins an ImGui frame. Updates delta time and display size.
	void NewFrame();

	/// Builds UI content (panels, overlays) and finalizes draw data.
	void Build();

	// ------------------------------------------------------------------------
	// Initialization Helpers
	// ------------------------------------------------------------------------

	/// Creates ImGui context and configures default settings.
	void InitializeImGuiContext();

	/// Initializes the Win32 platform backend.
	/// @return True if successful, false on failure.
	bool InitializeWin32Backend();

	/// Initializes the D3D12 rendering backend.
	/// @return True if successful, false on failure.
	bool InitializeD3D12Backend();

	/// Registers default UI panels and sections.
	void InitializeDefaultPanels();

	/// Subscribes to window message events for input handling.
	void SubscribeToWindowEvents(Window& window);

	/// Configures ImGui font and style for system DPI.
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
