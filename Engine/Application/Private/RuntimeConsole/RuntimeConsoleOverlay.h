#pragma once

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Events/ScopedEventHandle.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"

#include <Windows.h>
#include <array>
#include <d3d12.h>
#include <memory>
#include <string>
#include <vector>

class ConsoleSession;
class Timer;
class Window;
struct ImGui_ImplDX12_InitInfo;
struct ImGuiInputTextCallbackData;
struct ImVec4;
struct WindowMessageEvent;

class RuntimeConsoleOverlay final
{
  public:
	RuntimeConsoleOverlay(Timer& timer, Window& window, RenderHardwareInterface& renderHardware);
	~RuntimeConsoleOverlay() noexcept;

	RuntimeConsoleOverlay(const RuntimeConsoleOverlay&) = delete;
	RuntimeConsoleOverlay& operator=(const RuntimeConsoleOverlay&) = delete;
	RuntimeConsoleOverlay(RuntimeConsoleOverlay&&) = delete;
	RuntimeConsoleOverlay& operator=(RuntimeConsoleOverlay&&) = delete;

	void Update();
	void Render(NativeGraphicsCommandListHandle commandList) noexcept;
	bool IsVisible() const noexcept { return m_isVisible; }

  private:
	void HandleWindowMessage(WindowMessageEvent& event) noexcept;
	bool ProcessWindowMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	bool InitializeImGuiContext();
	bool InitializeWin32Backend();
	bool InitializeGraphicsBackend();
	bool InitializeNativeGraphicsBackend();
	void SetupDPIScaling() noexcept;
	bool IsReady() const noexcept;
	void ToggleVisibility() noexcept;

	void BuildUI();
	void DrawOutputRecords();
	void DrawInputLine();
	void DrawAutocompletePreview();
	void SubmitInput();
	std::vector<std::string> GetCurrentCompletions() const;
	std::string BuildCompletionList(const std::vector<std::string>& completions) const;

	int HandleHistoryCallback(ImGuiInputTextCallbackData& data);
	int HandleCompletionCallback(ImGuiInputTextCallbackData& data);

	static int HandleInputTextCallback(ImGuiInputTextCallbackData* data);
	static ImVec4 GetSeverityColor(ConsoleCommandSeverity severity) noexcept;
	static std::size_t FindCompletionTokenStart(const std::string& input) noexcept;
	static void ReplaceInputText(ImGuiInputTextCallbackData& data, const std::string& text);

	static ID3D12Device* ToD3D12Device(NativeGraphicsDeviceHandle handle) noexcept;
	static ID3D12CommandQueue* ToD3D12CommandQueue(NativeGraphicsQueueHandle handle) noexcept;
	static ID3D12DescriptorHeap* ToD3D12DescriptorHeap(NativeDescriptorHeapHandle handle) noexcept;
	static ID3D12GraphicsCommandList* ToD3D12GraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept;
	static D3D12_CPU_DESCRIPTOR_HANDLE ToD3D12CpuDescriptor(RhiCpuDescriptorHandle handle) noexcept;
	static D3D12_GPU_DESCRIPTOR_HANDLE ToD3D12GpuDescriptor(RhiGpuDescriptorHandle handle) noexcept;
	static void AllocateImGuiDescriptor(
	    ImGui_ImplDX12_InitInfo* info,
	    D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
	    D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
	static void ReleaseImGuiDescriptor(
	    ImGui_ImplDX12_InitInfo* info,
	    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	Timer* m_timer = nullptr;
	Window* m_window = nullptr;
	RenderHardwareInterface* m_renderHardware = nullptr;
	ScopedEventHandle m_windowMessageHandle;
	ConsoleCommandRegistry m_commandRegistry;
	std::unique_ptr<ConsoleSession> m_consoleSession;
	std::array<char, 512> m_inputBuffer{};
	std::array<char, 128> m_filterBuffer{};
	std::size_t m_seenOutputCount = 0;
	bool m_isVisible = false;
	bool m_scrollToBottom = true;
	bool m_focusInput = false;
	bool m_isImGuiContextInitialized = false;
	bool m_isWin32BackendInitialized = false;
	bool m_isGraphicsBackendInitialized = false;
};