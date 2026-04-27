#include "PCH.h"
#include "Panels/ConsolePanel.h"

#include "Core/Public/Console/ConsoleSession.h"
#include "Core/Public/Strings/StringUtils.h"

#include <imgui.h>

#include <algorithm>
#include <optional>

ConsolePanel::ConsolePanel(ConsoleSession& session) noexcept : m_session(&session) {}

void ConsolePanel::RequestFocus() noexcept
{
	m_isOpen = true;
	m_focusInput = true;
}

void ConsolePanel::BuildUI(bool disableInteraction)
{
	if (!m_isOpen || m_session == nullptr)
	{
		return;
	}

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	if (viewport != nullptr)
	{
		ImGui::SetNextWindowPos(
		    ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.45f, viewport->WorkPos.y + viewport->WorkSize.y - 320.0f),
		    ImGuiCond_FirstUseEver);
	}
	ImGui::SetNextWindowSize(ImVec2(760.0f, 300.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Console", &m_isOpen))
	{
		ImGui::End();
		return;
	}

	DrawToolbar(disableInteraction);
	ImGui::Separator();
	DrawOutputRecords();
	ImGui::Separator();
	DrawInputLine(disableInteraction);
	DrawAutocompletePreview();

	ImGui::End();
}

void ConsolePanel::BuildContent(bool disableInteraction)
{
	if (m_session == nullptr)
	{
		return;
	}

	DrawToolbar(disableInteraction);
	ImGui::Separator();
	DrawOutputRecords();
	ImGui::Separator();
	DrawInputLine(disableInteraction);
	DrawAutocompletePreview();
}

int ConsolePanel::HandleInputTextCallback(ImGuiInputTextCallbackData* data)
{
	if (data == nullptr || data->UserData == nullptr)
	{
		return 0;
	}

	auto* panel = static_cast<ConsolePanel*>(data->UserData);
	if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
	{
		return panel->HandleHistoryCallback(*data);
	}
	if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
	{
		return panel->HandleCompletionCallback(*data);
	}
	return 0;
}

ImVec4 ConsolePanel::GetSeverityColor(ConsoleCommandSeverity severity) noexcept
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

std::size_t ConsolePanel::FindCompletionTokenStart(const std::string& input) noexcept
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

void ConsolePanel::ReplaceInputText(ImGuiInputTextCallbackData& data, const std::string& text)
{
	data.DeleteChars(0, data.BufTextLen);
	data.InsertChars(0, text.c_str());
}

int ConsolePanel::HandleHistoryCallback(ImGuiInputTextCallbackData& data)
{
	if (m_session == nullptr)
	{
		return 0;
	}

	std::optional<std::string> replacement;
	if (data.EventKey == ImGuiKey_UpArrow)
	{
		replacement = m_session->NavigateHistoryPrevious(std::string_view(data.Buf, static_cast<std::size_t>(data.BufTextLen)));
	}
	else if (data.EventKey == ImGuiKey_DownArrow)
	{
		replacement = m_session->NavigateHistoryNext();
	}

	if (replacement)
	{
		ReplaceInputText(data, *replacement);
	}
	return 0;
}

int ConsolePanel::HandleCompletionCallback(ImGuiInputTextCallbackData& data)
{
	if (m_session == nullptr)
	{
		return 0;
	}

	const std::string input(data.Buf, static_cast<std::size_t>(data.BufTextLen));
	const std::vector<std::string> completions = m_session->CompleteLine(input);
	if (completions.empty())
	{
		return 0;
	}

	if (completions.size() > 1)
	{
		m_session->AddOutput(ConsoleCommandSeverity::Info, BuildCompletionList(completions));
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

void ConsolePanel::SubmitInput()
{
	if (m_session == nullptr)
	{
		return;
	}

	const std::string_view input(m_inputBuffer.data());
	m_session->SubmitLine(input);
	m_inputBuffer[0] = '\0';
	m_scrollToBottom = true;
	m_focusInput = true;
}

void ConsolePanel::DrawToolbar(bool disableInteraction)
{
	ImGui::BeginDisabled(disableInteraction);
	if (ImGui::Button("Help"))
	{
		m_session->SubmitLine("Help");
		m_scrollToBottom = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		m_session->ClearOutput();
		m_scrollToBottom = true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(220.0f);
	ImGui::InputTextWithHint("##ConsoleFilter", "Filter output", m_filterBuffer.data(), m_filterBuffer.size());
	ImGui::EndDisabled();
}

void ConsolePanel::DrawOutputRecords()
{
	const std::vector<ConsoleOutputRecord>& records = m_session->GetOutputRecords();
	if (records.size() != m_seenOutputCount)
	{
		m_seenOutputCount = records.size();
		m_scrollToBottom = true;
	}

	ImGui::BeginChild("##ConsoleScrollback", ImVec2(0.0f, -52.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
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

void ConsolePanel::DrawInputLine(bool disableInteraction)
{
	ImGui::BeginDisabled(disableInteraction);
	if (m_focusInput)
	{
		ImGui::SetKeyboardFocusHere();
		m_focusInput = false;
	}

	ImGui::SetNextItemWidth(-1.0f);
	const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
	    ImGuiInputTextFlags_CallbackHistory |
	    ImGuiInputTextFlags_CallbackCompletion;
	if (ImGui::InputTextWithHint("##ConsoleInput", "Type command, press Enter. Use Tab for completion.", m_inputBuffer.data(), m_inputBuffer.size(), flags, &HandleInputTextCallback, this))
	{
		SubmitInput();
	}
	ImGui::EndDisabled();
}

void ConsolePanel::DrawAutocompletePreview()
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

std::vector<std::string> ConsolePanel::GetCurrentCompletions() const
{
	if (m_session == nullptr || m_inputBuffer[0] == '\0')
	{
		return {};
	}
	return m_session->CompleteLine(m_inputBuffer.data());
}

std::string ConsolePanel::BuildCompletionList(const std::vector<std::string>& completions) const
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
