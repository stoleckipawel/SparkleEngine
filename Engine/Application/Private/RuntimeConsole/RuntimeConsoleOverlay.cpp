#include "PCH.h"
#include "RuntimeConsole/RuntimeConsoleOverlay.h"

#include "Core/Public/Console/ConsoleBuiltinCommands.h"
#include "Core/Public/Console/ConsoleSession.h"
#include "Core/Public/Strings/StringUtils.h"
#include "RHI/Public/UI/RhiImGuiRenderer.h"
#include "Time/Timer.h"
#include "Window/Window.h"

#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <algorithm>
#include <optional>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

RuntimeConsoleOverlay::RuntimeConsoleOverlay(Timer& timer, Window& window, RhiImGuiRenderer& imguiRenderer) :
	m_timer(&timer), m_window(&window), m_imguiRenderer(&imguiRenderer)
{
	ConsoleBuiltinCommands::Register(m_commandRegistry);
	m_consoleSession = std::make_unique<ConsoleSession>(m_commandRegistry, ConsoleCommandContext{.Scope = ConsoleCommandScope::Runtime});
	m_consoleSession->AddOutput(ConsoleCommandSeverity::Info, "Runtime console ready. Press tilde to close.");

	if (!InitializeImGuiContext())
	{
		return;
	}

	SetupDPIScaling();

	if (!InitializeWin32Backend())
	{
		return;
	}

	if (!InitializeGraphicsBackend())
	{
		return;
	}

	auto handle = window.OnWindowMessage.Add(
	    [this](WindowMessageEvent& event)
	    {
		    HandleWindowMessage(event);
	    });
	m_windowMessageHandle = ScopedEventHandle(window.OnWindowMessage, handle);
}

RuntimeConsoleOverlay::~RuntimeConsoleOverlay() noexcept
{
	m_windowMessageHandle.Reset();

	if (m_isGraphicsBackendInitialized)
	{
		m_imguiRenderer->Shutdown();
		m_isGraphicsBackendInitialized = false;
	}

	if (m_isWin32BackendInitialized)
	{
		ImGui_ImplWin32_Shutdown();
		m_isWin32BackendInitialized = false;
	}

	if (m_isImGuiContextInitialized)
	{
		ImGui::DestroyContext();
		m_isImGuiContextInitialized = false;
	}
}

void RuntimeConsoleOverlay::HandleWindowMessage(WindowMessageEvent& event) noexcept
{
	if (ProcessWindowMessage(event.hWnd, event.msg, event.wParam, event.lParam))
	{
		event.handled = true;
	}
}

bool RuntimeConsoleOverlay::ProcessWindowMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	if (!m_isWin32BackendInitialized)
	{
		return false;
	}

	if (msg == WM_KEYDOWN && wParam == VK_OEM_3)
	{
		const bool isRepeat = (lParam & (1 << 30)) != 0;
		if (!isRepeat)
		{
			ToggleVisibility();
			return true;
		}
	}

	if (!m_isVisible)
	{
		return false;
	}

	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) != 0;
}

bool RuntimeConsoleOverlay::InitializeImGuiContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	m_isImGuiContextInitialized = true;

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	return true;
}

bool RuntimeConsoleOverlay::InitializeWin32Backend()
{
	if (m_window == nullptr || !m_window->GetHWND())
	{
		return false;
	}

	ImGui_ImplWin32_Init(m_window->GetHWND());
	m_isWin32BackendInitialized = true;
	return true;
}

bool RuntimeConsoleOverlay::InitializeGraphicsBackend()
{
	if (m_imguiRenderer == nullptr)
	{
		return false;
	}

	m_isGraphicsBackendInitialized = m_imguiRenderer->Initialize();
	return m_isGraphicsBackendInitialized;
}

void RuntimeConsoleOverlay::SetupDPIScaling() noexcept
{
	ImGui_ImplWin32_EnableDpiAwareness();
	float mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
	ImGuiStyle& style = ImGui::GetStyle();
	style.FontSizeBase = 16.0f * mainScale;
	style.ScaleAllSizes(mainScale);
}

bool RuntimeConsoleOverlay::IsReady() const noexcept
{
	return m_isImGuiContextInitialized && m_isWin32BackendInitialized && m_isGraphicsBackendInitialized && m_consoleSession != nullptr;
}

void RuntimeConsoleOverlay::ToggleVisibility() noexcept
{
	m_isVisible = !m_isVisible;
	m_focusInput = m_isVisible;
}

void RuntimeConsoleOverlay::Update()
{
	if (!IsReady())
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = m_timer != nullptr ? static_cast<float>(m_timer->GetDelta(TimeDomain::Unscaled, TimeUnit::Seconds)) : (1.0f / 60.0f);
	if (m_window != nullptr)
	{
		io.DisplaySize = ImVec2(static_cast<float>(m_window->GetWidth()), static_cast<float>(m_window->GetHeight()));
	}

	m_imguiRenderer->BeginFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	if (m_isVisible)
	{
		BuildUI();
	}
	ImGui::Render();
}

void RuntimeConsoleOverlay::Render() noexcept
{
	if (!IsReady() || !m_isVisible)
	{
		return;
	}

	m_imguiRenderer->RenderDrawData(ImGui::GetDrawData());
}

void RuntimeConsoleOverlay::BuildUI()
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	if (viewport != nullptr)
	{
		const float width = (std::max) (640.0f, viewport->WorkSize.x * 0.55f);
		ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 24.0f, viewport->WorkPos.y + viewport->WorkSize.y - 340.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, 316.0f), ImGuiCond_Always);
	}

	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
	if (!ImGui::Begin("Runtime Console", &m_isVisible, flags))
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button("Help"))
	{
		m_consoleSession->SubmitLine("Help");
		m_scrollToBottom = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		m_consoleSession->ClearOutput();
		m_scrollToBottom = true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(220.0f);
	ImGui::InputTextWithHint("##RuntimeConsoleFilter", "Filter output", m_filterBuffer.data(), m_filterBuffer.size());
	ImGui::SameLine();
	ImGui::TextDisabled("Tilde toggles");

	ImGui::Separator();
	DrawOutputRecords();
	ImGui::Separator();
	DrawInputLine();
	DrawAutocompletePreview();

	ImGui::End();
}

void RuntimeConsoleOverlay::DrawOutputRecords()
{
	const std::vector<ConsoleOutputRecord>& records = m_consoleSession->GetOutputRecords();
	if (records.size() != m_seenOutputCount)
	{
		m_seenOutputCount = records.size();
		m_scrollToBottom = true;
	}

	ImGui::BeginChild("##RuntimeConsoleScrollback", ImVec2(0.0f, -52.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	const std::string_view filter(m_filterBuffer.data());
	for (const ConsoleOutputRecord& record : records)
	{
		if (!filter.empty() && !Strings::ContainsIgnoreCase(record.Text, filter))
		{
			continue;
		}

		ImGui::PushStyleColor(ImGuiCol_Text, GetSeverityColor(record.Severity));
		ImGui::TextUnformatted(record.Text.c_str());
		ImGui::PopStyleColor();
	}

	if (m_scrollToBottom)
	{
		ImGui::SetScrollHereY(1.0f);
		m_scrollToBottom = false;
	}
	ImGui::EndChild();
}

void RuntimeConsoleOverlay::DrawInputLine()
{
	if (m_focusInput)
	{
		ImGui::SetKeyboardFocusHere();
		m_focusInput = false;
	}

	ImGui::SetNextItemWidth(-1.0f);
	const ImGuiInputTextFlags flags =
	    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion;
	if (ImGui::InputTextWithHint(
	        "##RuntimeConsoleInput",
	        "Type command, press Enter. Use Tab for completion.",
	        m_inputBuffer.data(),
	        m_inputBuffer.size(),
	        flags,
	        &HandleInputTextCallback,
	        this))
	{
		SubmitInput();
	}
}

void RuntimeConsoleOverlay::DrawAutocompletePreview()
{
	const std::vector<std::string> completions = GetCurrentCompletions();
	if (completions.empty())
	{
		return;
	}

	ImGui::TextDisabled("Matches: ");
	ImGui::SameLine();
	const std::size_t maxPreviewCount = (std::min) (completions.size(), static_cast<std::size_t>(6));
	for (std::size_t index = 0; index < maxPreviewCount; ++index)
	{
		if (index > 0)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("|");
			ImGui::SameLine();
		}
		ImGui::TextDisabled("%s", completions[index].c_str());
	}
	if (completions.size() > maxPreviewCount)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(+%zu)", completions.size() - maxPreviewCount);
	}
}

void RuntimeConsoleOverlay::SubmitInput()
{
	const std::string_view input(m_inputBuffer.data());
	m_consoleSession->SubmitLine(input);
	m_inputBuffer[0] = '\0';
	m_scrollToBottom = true;
	m_focusInput = true;
}

std::vector<std::string> RuntimeConsoleOverlay::GetCurrentCompletions() const
{
	if (m_consoleSession == nullptr || m_inputBuffer[0] == '\0')
	{
		return {};
	}
	return m_consoleSession->CompleteLine(m_inputBuffer.data());
}

std::string RuntimeConsoleOverlay::BuildCompletionList(const std::vector<std::string>& completions) const
{
	std::string output = "matches:";
	const std::size_t maxOutputCount = (std::min) (completions.size(), static_cast<std::size_t>(12));
	for (std::size_t index = 0; index < maxOutputCount; ++index)
	{
		output += ' ';
		output += completions[index];
	}
	if (completions.size() > maxOutputCount)
	{
		output += " ...";
	}
	return output;
}

int RuntimeConsoleOverlay::HandleInputTextCallback(ImGuiInputTextCallbackData* data)
{
	if (data == nullptr || data->UserData == nullptr)
	{
		return 0;
	}

	auto* overlay = static_cast<RuntimeConsoleOverlay*>(data->UserData);
	if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
	{
		return overlay->HandleHistoryCallback(*data);
	}
	if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
	{
		return overlay->HandleCompletionCallback(*data);
	}
	return 0;
}

int RuntimeConsoleOverlay::HandleHistoryCallback(ImGuiInputTextCallbackData& data)
{
	std::optional<std::string> replacement;
	if (data.EventKey == ImGuiKey_UpArrow)
	{
		replacement = m_consoleSession->NavigateHistoryPrevious(std::string_view(data.Buf, static_cast<std::size_t>(data.BufTextLen)));
	}
	else if (data.EventKey == ImGuiKey_DownArrow)
	{
		replacement = m_consoleSession->NavigateHistoryNext();
	}

	if (replacement)
	{
		ReplaceInputText(data, *replacement);
	}
	return 0;
}

int RuntimeConsoleOverlay::HandleCompletionCallback(ImGuiInputTextCallbackData& data)
{
	const std::string input(data.Buf, static_cast<std::size_t>(data.BufTextLen));
	const std::vector<std::string> completions = m_consoleSession->CompleteLine(input);
	if (completions.empty())
	{
		return 0;
	}

	if (completions.size() > 1)
	{
		m_consoleSession->AddOutput(ConsoleCommandSeverity::Info, BuildCompletionList(completions));
		m_scrollToBottom = true;
		return 0;
	}

	const std::size_t replaceStart = FindCompletionTokenStart(input);
	data.DeleteChars(static_cast<int>(replaceStart), data.BufTextLen - static_cast<int>(replaceStart));
	data.InsertChars(static_cast<int>(replaceStart), completions.front().c_str());
	if (replaceStart == 0)
	{
		data.InsertChars(data.BufTextLen, " ");
	}
	return 0;
}

ImVec4 RuntimeConsoleOverlay::GetSeverityColor(ConsoleCommandSeverity severity) noexcept
{
	switch (severity)
	{
		case ConsoleCommandSeverity::Warning:
			return ImVec4(1.0f, 0.78f, 0.28f, 1.0f);
		case ConsoleCommandSeverity::Error:
			return ImVec4(1.0f, 0.34f, 0.30f, 1.0f);
		case ConsoleCommandSeverity::Info:
		default:
			return ImVec4(0.86f, 0.88f, 0.92f, 1.0f);
	}
}

std::size_t RuntimeConsoleOverlay::FindCompletionTokenStart(const std::string& input) noexcept
{
	for (std::size_t index = input.size(); index > 0; --index)
	{
		if (Strings::IsAsciiWhitespace(input[index - 1]))
		{
			return index;
		}
	}
	return 0;
}

void RuntimeConsoleOverlay::ReplaceInputText(ImGuiInputTextCallbackData& data, const std::string& text)
{
	data.DeleteChars(0, data.BufTextLen);
	data.InsertChars(0, text.c_str());
}
