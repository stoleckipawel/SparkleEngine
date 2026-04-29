#include "PCH.h"
#include "Panels/OutputLogPanel.h"

#include "Core/Public/Console/ConsoleSession.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Style/SparkleUiPalette.h"
#include "Style/SparkleUiTheme.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <algorithm>
#include <iterator>
#include <optional>

namespace
{
	constexpr std::size_t kMaxOutputLogRecords = 2048;
}

OutputLogPanel::OutputLogPanel(ConsoleSession& session) noexcept : m_session(&session) {}

void OutputLogPanel::RequestFocus() noexcept
{
	m_isOpen = true;
	m_focusInput = true;
}

void OutputLogPanel::AddLine(ConsoleCommandSeverity severity, std::string text)
{
	AddRecord(ConsoleOutputRecord{.Severity = severity, .Text = std::move(text)});
	DrainPendingRecords();
}

void OutputLogPanel::AddRecord(ConsoleOutputRecord record)
{
	std::lock_guard<std::mutex> lock(m_recordsMutex);
	m_pendingRecords.push_back(std::move(record));
}

void OutputLogPanel::Clear() noexcept
{
	std::lock_guard<std::mutex> lock(m_recordsMutex);
	m_records.clear();
	m_pendingRecords.clear();
	if (m_session != nullptr)
	{
		m_session->ClearOutput();
		m_seenConsoleOutputCount = 0;
	}
	m_scrollToBottom = true;
}

void OutputLogPanel::BuildUI(bool disableInteraction)
{
	DrainPendingRecords();

	if (!m_isOpen)
	{
		return;
	}

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	if (viewport != nullptr)
	{
		ImGui::SetNextWindowPos(
		    ImVec2(viewport->WorkPos.x + 12.0f, viewport->WorkPos.y + viewport->WorkSize.y - 320.0f),
		    ImGuiCond_FirstUseEver);
	}
	ImGui::SetNextWindowSize(ImVec2(600.0f, 300.0f), ImGuiCond_FirstUseEver);
	const std::string windowTitle = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Console, "Output Log") + "##Output Log";
	if (!ImGui::Begin(windowTitle.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	DrawToolbar(disableInteraction);
	ImGui::Separator();
	DrawScrollback();
	ImGui::Separator();
	DrawInputLine(disableInteraction);
	DrawAutocompletePreview();
	ImGui::End();
}

void OutputLogPanel::BuildContent(bool disableInteraction)
{
	DrainPendingRecords();

	DrawToolbar(disableInteraction);
	ImGui::Separator();
	DrawScrollback();
	ImGui::Separator();
	DrawInputLine(disableInteraction);
	DrawAutocompletePreview();
}

int OutputLogPanel::HandleInputTextCallback(ImGuiInputTextCallbackData* data)
{
	if (data == nullptr || data->UserData == nullptr)
	{
		return 0;
	}

	auto* panel = static_cast<OutputLogPanel*>(data->UserData);
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

std::size_t OutputLogPanel::FindCompletionTokenStart(const std::string& input) noexcept
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

void OutputLogPanel::ReplaceInputText(ImGuiInputTextCallbackData& data, const std::string& text)
{
	data.DeleteChars(0, data.BufTextLen);
	data.InsertChars(0, text.c_str());
}

int OutputLogPanel::HandleHistoryCallback(ImGuiInputTextCallbackData& data)
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

int OutputLogPanel::HandleCompletionCallback(ImGuiInputTextCallbackData& data)
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

void OutputLogPanel::SubmitInput()
{
	if (m_session == nullptr)
	{
		return;
	}

	const std::string_view input(m_inputBuffer.data());
	m_session->SubmitLine(input);
	m_inputBuffer[0] = '\0';
	m_seenConsoleOutputCount = m_session->GetOutputRecords().size();
	m_scrollToBottom = true;
	m_focusInput = true;
}

void OutputLogPanel::DrawToolbar(bool disableInteraction)
{
	ImGui::BeginDisabled(disableInteraction);
	if (ImGui::BeginTable("##OutputLogToolbar", 5, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX))
	{
		ImGui::TableSetupColumn("help", ImGuiTableColumnFlags_WidthFixed, 66.0f);
		ImGui::TableSetupColumn("clear", ImGuiTableColumnFlags_WidthFixed, 66.0f);
		ImGui::TableSetupColumn("copy", ImGuiTableColumnFlags_WidthFixed, 66.0f);
		ImGui::TableSetupColumn("filter", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("count", ImGuiTableColumnFlags_WidthFixed, 96.0f);
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		const std::string helpLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Help, "Help");
		if (ImGui::Button(helpLabel.c_str(), ImVec2(-1.0f, 0.0f)) && m_session != nullptr)
		{
			m_session->SubmitLine("Help");
			m_scrollToBottom = true;
		}

		ImGui::TableSetColumnIndex(1);
		const std::string clearLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Clear, "Clear");
		if (ImGui::Button(clearLabel.c_str(), ImVec2(-1.0f, 0.0f)))
		{
			Clear();
		}

		ImGui::TableSetColumnIndex(2);
		const std::string copyLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Copy, "Copy");
		if (ImGui::Button(copyLabel.c_str(), ImVec2(-1.0f, 0.0f)))
		{
			BuildOutputTextBuffer(std::string_view(m_filterBuffer.data()));
			ImGui::SetClipboardText(m_outputTextBuffer.c_str());
		}

		ImGui::TableSetColumnIndex(3);
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##OutputLogFilter", "Filter output", m_filterBuffer.data(), m_filterBuffer.size());

		ImGui::TableSetColumnIndex(4);
		const std::size_t consoleRecordCount = m_session != nullptr ? m_session->GetOutputRecords().size() : 0;
		ImGui::TextDisabled("%zu records", m_records.size() + consoleRecordCount);
		ImGui::EndTable();
	}
	ImGui::EndDisabled();
}

void OutputLogPanel::DrawScrollback()
{
	if (m_session != nullptr && m_session->GetOutputRecords().size() != m_seenConsoleOutputCount)
	{
		m_seenConsoleOutputCount = m_session->GetOutputRecords().size();
		m_scrollToBottom = true;
	}

	const std::string_view filter(m_filterBuffer.data());
	BuildOutputTextBuffer(filter);

	ImFont* monoFont = SparkleUiTheme::GetMonoFont();
	if (monoFont != nullptr)
	{
		ImGui::PushFont(monoFont);
	}
	ImGui::PushStyleColor(ImGuiCol_FrameBg, SparkleUiPalette::ConsoleScrollbackBackground());
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, SparkleUiPalette::ConsoleScrollbackBackground());
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, SparkleUiPalette::ConsoleScrollbackBackground());
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
	const ImGuiInputTextFlags flags = ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo;
	const ImVec2 available = ImGui::GetContentRegionAvail();
	ImGui::InputTextMultiline(
	    "##OutputLogScrollback",
	    m_outputTextBuffer.data(),
	    m_outputTextBuffer.size(),
	    ImVec2(available.x, (std::max) (32.0f, available.y - 34.0f)),
	    flags);
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);
	if (monoFont != nullptr)
	{
		ImGui::PopFont();
	}
	if (m_scrollToBottom)
	{
		m_scrollToBottom = false;
	}
}

void OutputLogPanel::DrawInputLine(bool disableInteraction)
{
	ImGui::BeginDisabled(disableInteraction || m_session == nullptr);
	if (m_focusInput)
	{
		ImGui::SetKeyboardFocusHere();
		m_focusInput = false;
	}

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, SparkleUiPalette::ConsoleInputBackground());
	const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
	    ImGuiInputTextFlags_CallbackHistory |
	    ImGuiInputTextFlags_CallbackCompletion;
	if (ImGui::InputTextWithHint("##OutputLogCommandInput", "Enter command", m_inputBuffer.data(), m_inputBuffer.size(), flags, &HandleInputTextCallback, this))
	{
		SubmitInput();
	}
	ImGui::PopStyleColor();
	ImGui::EndDisabled();
}

void OutputLogPanel::DrawAutocompletePreview()
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

void OutputLogPanel::BuildOutputTextBuffer(std::string_view filter)
{
	m_outputTextBuffer.clear();
	for (const ConsoleOutputRecord& record : m_records)
	{
		if (!filter.empty() && !Strings::ContainsIgnoreCase(record.Text, filter))
		{
			continue;
		}

		m_outputTextBuffer += record.Text;
		m_outputTextBuffer += '\n';
	}

	if (m_session != nullptr)
	{
		for (const ConsoleOutputRecord& record : m_session->GetOutputRecords())
		{
			if (!filter.empty() && !Strings::ContainsIgnoreCase(record.Text, filter))
			{
				continue;
			}

			m_outputTextBuffer += record.Text;
			m_outputTextBuffer += '\n';
		}
	}

	if (m_outputTextBuffer.empty())
	{
		m_outputTextBuffer.push_back('\0');
		return;
	}
	m_outputTextBuffer.push_back('\0');
}

void OutputLogPanel::DrainPendingRecords()
{
	std::vector<ConsoleOutputRecord> pendingRecords;
	{
		std::lock_guard<std::mutex> lock(m_recordsMutex);
		pendingRecords.swap(m_pendingRecords);
	}

	if (pendingRecords.empty())
	{
		return;
	}

	m_records.insert(
	    m_records.end(),
	    std::make_move_iterator(pendingRecords.begin()),
	    std::make_move_iterator(pendingRecords.end()));
	while (m_records.size() > kMaxOutputLogRecords)
	{
		m_records.erase(m_records.begin(), m_records.begin() + (std::min) (m_records.size() - kMaxOutputLogRecords, m_records.size()));
	}
	m_scrollToBottom = true;
}

std::vector<std::string> OutputLogPanel::GetCurrentCompletions() const
{
	if (m_session == nullptr || m_inputBuffer[0] == '\0')
	{
		return {};
	}
	return m_session->CompleteLine(m_inputBuffer.data());
}

std::string OutputLogPanel::BuildCompletionList(const std::vector<std::string>& completions) const
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
