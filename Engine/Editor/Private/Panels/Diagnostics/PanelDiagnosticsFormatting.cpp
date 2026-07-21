#include "PCH.h"

#include "PanelDiagnosticsFormatting.h"

#include <format>

#include <imgui.h>

namespace PanelDiagnosticsFormatting
{
	std::string FormatByteSize(std::uint64_t bytes)
	{
		constexpr double KiB = 1024.0;
		constexpr double MiB = KiB * 1024.0;
		constexpr double GiB = MiB * 1024.0;
		if (bytes >= static_cast<std::uint64_t>(GiB)) return std::format("{:.2f} GiB", static_cast<double>(bytes) / GiB);
		if (bytes >= static_cast<std::uint64_t>(MiB)) return std::format("{:.2f} MiB", static_cast<double>(bytes) / MiB);
		if (bytes >= static_cast<std::uint64_t>(KiB)) return std::format("{:.1f} KiB", static_cast<double>(bytes) / KiB);
		return std::format("{} B", bytes);
	}

	void DrawWrappedDisabledText(const std::string& text)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(text.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
	}
}
