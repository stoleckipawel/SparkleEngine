#include "PCH.h"
#include "Panels/OutputLogPanel.h"

#include "Core/Public/Strings/StringUtils.h"

#include <imgui.h>

#include <algorithm>
#include <iterator>

namespace
{
	constexpr std::size_t kMaxOutputLogRecords = 2048;
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
	if (!ImGui::Begin("Output Log", &m_isOpen))
	{
		ImGui::End();
		return;
	}

	ImGui::BeginDisabled(disableInteraction);
	if (ImGui::Button("Clear"))
	{
		Clear();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(220.0f);
	ImGui::InputTextWithHint("##OutputLogFilter", "Filter log output", m_filterBuffer.data(), m_filterBuffer.size());
	ImGui::SameLine();
	ImGui::TextDisabled("%zu record(s)", m_records.size());
	ImGui::EndDisabled();
	ImGui::Separator();

	const std::string_view filter(m_filterBuffer.data());
	ImGui::BeginChild("##OutputLogScrollback", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	for (const ConsoleOutputRecord& record : m_records)
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
	ImGui::End();
}

void OutputLogPanel::BuildContent(bool disableInteraction)
{
	DrainPendingRecords();

	ImGui::BeginDisabled(disableInteraction);
	if (ImGui::Button("Clear"))
	{
		Clear();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(220.0f);
	ImGui::InputTextWithHint("##OutputLogFilter", "Filter log output", m_filterBuffer.data(), m_filterBuffer.size());
	ImGui::SameLine();
	ImGui::TextDisabled("%zu record(s)", m_records.size());
	ImGui::EndDisabled();
	ImGui::Separator();

	const std::string_view filter(m_filterBuffer.data());
	ImGui::BeginChild("##OutputLogScrollback", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	for (const ConsoleOutputRecord& record : m_records)
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

ImVec4 OutputLogPanel::GetSeverityColor(ConsoleCommandSeverity severity) noexcept
{
	switch (severity)
	{
		case ConsoleCommandSeverity::Warning:
			return ImVec4(1.0f, 0.78f, 0.28f, 1.0f);
		case ConsoleCommandSeverity::Error:
			return ImVec4(1.0f, 0.34f, 0.30f, 1.0f);
		case ConsoleCommandSeverity::Info:
		default:
			return ImVec4(0.78f, 0.82f, 0.88f, 1.0f);
	}
}
