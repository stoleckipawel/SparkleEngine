#include "PCH.h"

#include "Panels/UsedShadersPanel.h"

#include "Core/Public/Strings/StringUtils.h"

#include <imgui.h>

#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

void UsedShadersPanel::SetGenerationProvider(RegisteredShaderListModel::GenerationProvider provider)
{
	m_model.SetGenerationProvider(std::move(provider));
	m_hasRows = false;
}

void UsedShadersPanel::SetReloadHandler(CommandHandler handler)
{
	m_reloadHandler = std::move(handler);
}

void UsedShadersPanel::SetRecookAllHandler(CommandHandler handler)
{
	m_recookAllHandler = std::move(handler);
}

void UsedShadersPanel::SetRecookHandler(RecookHandler handler)
{
	m_recookHandler = std::move(handler);
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
	ImGui::SetNextWindowSize(ImVec2(1180.0f, 760.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Shader Tools", &m_isOpen))
	{
		ImGui::End();
		return;
	}

	DrawToolbar(disableInteraction);
	ImGui::Separator();
	const float availableHeight = ImGui::GetContentRegionAvail().y;
	const float tableHeight = availableHeight * 0.48f;
	ImGui::BeginChild("##UsedShadersTableRegion", ImVec2(0.0f, tableHeight), ImGuiChildFlags_None);
	DrawTable(disableInteraction);
	ImGui::EndChild();
	ImGui::SeparatorText("Selected Shader Inspection");
	DrawSelectedShaderArtifacts();
	ImGui::End();
}

std::filesystem::path UsedShadersPanel::FindArtifactDirectory(const RegisteredShaderRow& row)
{
	if (!row.ArtifactDirectory.empty())
	{
		std::error_code errorCode;
		if (std::filesystem::exists(row.ArtifactDirectory / "compile-request.json", errorCode) && !errorCode)
		{
			return row.ArtifactDirectory;
		}
	}

	return {};
}

std::string UsedShadersPanel::ReadTextFileOrMessage(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input.is_open())
	{
		return "Artifact not found: " + path.generic_string();
	}

	std::ostringstream contents;
	contents << input.rdbuf();
	return contents.str();
}

void UsedShadersPanel::DrawTextArtifact(const char* childId, const std::string& text) noexcept
{
	ImGui::BeginChild(childId, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::InputTextMultiline("##ArtifactText", const_cast<char*>(text.c_str()), text.size() + 1, ImGui::GetContentRegionAvail(),
	    ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_AllowTabInput);
	ImGui::EndChild();
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
		m_loadedArtifactShaderId.clear();
		RefreshSelectedShaderArtifacts();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload Cooked") && m_reloadHandler)
	{
		m_reloadHandler();
	}
	ImGui::SameLine();
	if (ImGui::Button("Recook All") && m_recookAllHandler)
	{
		m_recookAllHandler();
	}
	ImGui::SameLine();

	const RegisteredShaderRow* selectedRow = GetSelectedRow();
	if (ImGui::Button("Recook Selected") && selectedRow != nullptr && m_recookHandler)
	{
		m_recookHandler(selectedRow->PackageId);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputTextWithHint("##UsedShadersFilter", "Filter shader/package/source", m_filterBuffer.data(), m_filterBuffer.size());
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
			RefreshSelectedShaderArtifacts();
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

void UsedShadersPanel::DrawSelectedShaderArtifacts()
{
	const RegisteredShaderRow* selectedRow = GetSelectedRow();
	if (selectedRow == nullptr)
	{
		ImGui::TextDisabled("Select a shader row to inspect source, reflection, disassembly, parameter match, and compile request artifacts.");
		return;
	}

	RefreshSelectedShaderArtifacts();
	ImGui::TextUnformatted(selectedRow->ShaderId.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("%s | %s | %s", selectedRow->PackageId.c_str(), selectedRow->Stage.c_str(), selectedRow->EntryPoint.c_str());
	if (m_selectedArtifactDirectory.empty())
	{
		ImGui::TextWrapped("No debug artifact bundle is available for this shader. Recook shaders with debug artifacts enabled to populate inspection data.");
		return;
	}

	ImGui::TextDisabled("%s", m_selectedArtifactDirectory.generic_string().c_str());
	if (ImGui::BeginTabBar("##SelectedShaderArtifactTabs"))
	{
		if (ImGui::BeginTabItem("Source"))
		{
			DrawTextArtifact("##SelectedShaderSourceText", m_artifactTexts.Source);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Reflection"))
		{
			DrawTextArtifact("##SelectedShaderReflectionText", m_artifactTexts.Reflection);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Disassembly"))
		{
			DrawTextArtifact("##SelectedShaderDisassemblyText", m_artifactTexts.Disassembly);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Param Match"))
		{
			DrawTextArtifact("##SelectedShaderParamMatchText", m_artifactTexts.ParameterMatch);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Compile Request"))
		{
			DrawTextArtifact("##SelectedShaderCompileRequestText", m_artifactTexts.CompileRequest);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void UsedShadersPanel::RefreshSelectedShaderArtifacts()
{
	const RegisteredShaderRow* selectedRow = GetSelectedRow();
	if (selectedRow == nullptr)
	{
		m_loadedArtifactShaderId.clear();
		m_selectedArtifactDirectory.clear();
		m_artifactTexts = {};
		return;
	}

	if (m_loadedArtifactShaderId == selectedRow->ShaderId)
	{
		return;
	}

	m_loadedArtifactShaderId = selectedRow->ShaderId;
	m_selectedArtifactDirectory = FindArtifactDirectory(*selectedRow);
	if (m_selectedArtifactDirectory.empty())
	{
		m_artifactTexts = {};
		return;
	}

	m_artifactTexts.Source = ReadTextFileOrMessage(m_selectedArtifactDirectory / "preprocessed-source.hlsl");
	m_artifactTexts.Reflection = ReadTextFileOrMessage(m_selectedArtifactDirectory / "reflection.json");
	m_artifactTexts.Disassembly = ReadTextFileOrMessage(m_selectedArtifactDirectory / "disassembly.txt");
	m_artifactTexts.ParameterMatch = ReadTextFileOrMessage(m_selectedArtifactDirectory / "parameter-struct-match.json");
	m_artifactTexts.CompileRequest = ReadTextFileOrMessage(m_selectedArtifactDirectory / "compile-request.json");
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
