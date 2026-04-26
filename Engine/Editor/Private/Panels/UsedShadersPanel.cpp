#include "PCH.h"

#include "Panels/UsedShadersPanel.h"

#include "Core/Public/Strings/StringUtils.h"

#include <imgui.h>

#include <utility>

void UsedShadersPanel::SetGenerationProvider(RegisteredShaderListModel::GenerationProvider provider)
{
	m_model.SetGenerationProvider(std::move(provider));
	m_hasRows = false;
}

void UsedShadersPanel::SetRecookHandler(RecookHandler handler)
{
	m_recookHandler = std::move(handler);
}

void UsedShadersPanel::SetInspectHandler(InspectHandler handler)
{
	m_inspectHandler = std::move(handler);
}

void UsedShadersPanel::SetLastStatus(std::string status)
{
	m_model.SetLastStatus(std::move(status));
}

void UsedShadersPanel::BuildUI(bool disableInteraction)
{
	if (!m_isOpen)
	{
		return;
	}

	EnsureRows();
	ImGui::SetNextWindowSize(ImVec2(1080.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Used Shaders", &m_isOpen))
	{
		ImGui::End();
		return;
	}

	DrawToolbar(disableInteraction);
	ImGui::Separator();
	DrawTable(disableInteraction);
	ImGui::End();
}

void UsedShadersPanel::EnsureRows()
{
	if (m_hasRows)
	{
		return;
	}

	m_model.Refresh();
	m_hasRows = true;
}

void UsedShadersPanel::DrawToolbar(bool disableInteraction)
{
	ImGui::BeginDisabled(disableInteraction);
	if (ImGui::Button("Refresh"))
	{
		m_model.Refresh();
		m_hasRows = true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputTextWithHint("##UsedShadersFilter", "Filter shader/package/source", m_filterBuffer.data(), m_filterBuffer.size());
	ImGui::SameLine();

	const RegisteredShaderRow* selectedRow = GetSelectedRow();
	if (ImGui::Button("Recook Selected") && selectedRow != nullptr && m_recookHandler)
	{
		m_recookHandler(selectedRow->PackageId);
	}
	ImGui::SameLine();
	if (ImGui::Button("Inspect Artifacts") && m_inspectHandler)
	{
		m_inspectHandler();
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy Command") && selectedRow != nullptr)
	{
		const std::string command = "RecompileShaders " + selectedRow->PackageId;
		ImGui::SetClipboardText(command.c_str());
	}
	ImGui::SameLine();
	ImGui::TextDisabled("%zu registered shader stage(s)", m_model.GetRows().size());
	ImGui::EndDisabled();
}

void UsedShadersPanel::DrawTable(bool disableInteraction)
{
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders |
	    ImGuiTableFlags_RowBg |
	    ImGuiTableFlags_Resizable |
	    ImGuiTableFlags_Reorderable |
	    ImGuiTableFlags_ScrollX |
	    ImGuiTableFlags_ScrollY;
	if (!ImGui::BeginTable("##UsedShadersTable", 12, tableFlags, ImVec2(0.0f, 0.0f)))
	{
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Shader Id");
	ImGui::TableSetupColumn("Package Id");
	ImGui::TableSetupColumn("Stage");
	ImGui::TableSetupColumn("Source Path");
	ImGui::TableSetupColumn("Entry");
	ImGui::TableSetupColumn("Layout");
	ImGui::TableSetupColumn("Params");
	ImGui::TableSetupColumn("Permutations");
	ImGui::TableSetupColumn("Backend/Target");
	ImGui::TableSetupColumn("Generation");
	ImGui::TableSetupColumn("Artifacts");
	ImGui::TableSetupColumn("Last Recook Status");
	ImGui::TableHeadersRow();

	ImGui::BeginDisabled(disableInteraction);
	for (const RegisteredShaderRow& row : m_model.GetRows())
	{
		if (!MatchesFilter(row))
		{
			continue;
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		const bool selected = row.ShaderId == m_selectedShaderId;
		if (ImGui::Selectable(row.ShaderId.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
		{
			m_selectedShaderId = row.ShaderId;
		}
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.PackageId.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.Stage.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.SourcePath.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.EntryPoint.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.BindingLayoutId.c_str());
		ImGui::TableNextColumn();
		ImGui::Text("%zu", row.ParameterCount);
		ImGui::TableNextColumn();
		ImGui::Text("%zu", row.PermutationDimensionCount);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("auto / DxilSm66");
		ImGui::TableNextColumn();
		ImGui::Text("%llu", static_cast<unsigned long long>(row.RuntimeGeneration));
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.ArtifactAvailable ? "available" : "missing");
		ImGui::TableNextColumn();
		ImGui::TextWrapped("%s", row.LastStatus.c_str());
	}
	ImGui::EndDisabled();
	ImGui::EndTable();
}

const RegisteredShaderRow* UsedShadersPanel::GetSelectedRow() const noexcept
{
	for (const RegisteredShaderRow& row : m_model.GetRows())
	{
		if (row.ShaderId == m_selectedShaderId)
		{
			return &row;
		}
	}
	return nullptr;
}

bool UsedShadersPanel::MatchesFilter(const RegisteredShaderRow& row) const noexcept
{
	const std::string_view filter(m_filterBuffer.data());
	return filter.empty() ||
	    Strings::ContainsIgnoreCase(row.ShaderId, filter) ||
	    Strings::ContainsIgnoreCase(row.PackageId, filter) ||
	    Strings::ContainsIgnoreCase(row.SourcePath, filter) ||
	    Strings::ContainsIgnoreCase(row.Stage, filter);
}
