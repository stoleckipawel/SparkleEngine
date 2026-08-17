#pragma once

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Events/ScopedEventHandle.h"
#include "Renderer/Public/UI/UiRenderPacket.h"

#include <Windows.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

class ConsoleSession;
class ImGuiRenderPacketBuilder;
class Timer;
class Window;
struct ImGuiInputTextCallbackData;
struct ImVec4;
struct WindowMessageEvent;

class RuntimeConsoleOverlay final
{
public:
	RuntimeConsoleOverlay(Timer& timer, Window& window);
	~RuntimeConsoleOverlay() noexcept;

	RuntimeConsoleOverlay(const RuntimeConsoleOverlay&) = delete;
	RuntimeConsoleOverlay& operator=(const RuntimeConsoleOverlay&) = delete;
	RuntimeConsoleOverlay(RuntimeConsoleOverlay&&) = delete;
	RuntimeConsoleOverlay& operator=(RuntimeConsoleOverlay&&) = delete;

	void Update();
	UiRenderPacket ConsumeRenderPacket();
	bool IsVisible() const noexcept { return m_isVisible; }

private:
	void HandleWindowMessage(WindowMessageEvent& event) noexcept;
	bool ProcessWindowMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	bool InitializeImGuiContext();
	bool InitializeWin32Backend();
	void ApplyDpiScale(float dpiScale) noexcept;
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

	Timer* m_timer = nullptr;
	Window* m_window = nullptr;
	ScopedEventHandle m_windowMessageHandle;
	ScopedEventHandle m_windowDpiScaleHandle;
	ConsoleCommandRegistry m_commandRegistry;
	std::unique_ptr<ConsoleSession> m_consoleSession;
	std::unique_ptr<ImGuiRenderPacketBuilder> m_renderPacketBuilder;
	UiRenderPacket m_renderPacket;
	std::array<char, 512> m_inputBuffer{};
	std::array<char, 128> m_filterBuffer{};
	std::size_t m_seenOutputCount = 0;
	bool m_isVisible = false;
	bool m_scrollToBottom = true;
	bool m_focusInput = false;
	bool m_isImGuiContextInitialized = false;
	bool m_isWin32BackendInitialized = false;
};
