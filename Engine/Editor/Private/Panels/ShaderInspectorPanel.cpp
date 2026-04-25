#include "PCH.h"
#include "Panels/ShaderInspectorPanel.h"

#include "Core/Public/FileSystemUtils.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <system_error>

#include <imgui.h>

std::filesystem::path ShaderInspectorPanel::GetArtifactSearchRoot()
{
	return Filesystem::GetExecutableDirectory().parent_path() / "Cache" / "Shaders";
}

std::string ShaderInspectorPanel::BuildBundleLabel(const std::filesystem::path& directory)
{
	std::string label = directory.filename().string();
	std::replace(label.begin(), label.end(), '_', ' ');
	return label;
}

std::string ShaderInspectorPanel::ReadTextFileOrMessage(const std::filesystem::path& path)
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

void ShaderInspectorPanel::DrawTextArtifact(const char* childId, const std::string& text) noexcept
{
	ImGui::BeginChild(childId, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::TextUnformatted(text.c_str());
	ImGui::EndChild();
}

void ShaderInspectorPanel::RefreshBundles()
{
	m_bundles.clear();
	m_hasScanned = true;
	m_hasSelection = false;
	m_selectedIndex = 0;
	m_texts = {};

	const std::filesystem::path root = GetArtifactSearchRoot();
	std::error_code errorCode;
	if (!std::filesystem::exists(root, errorCode) || errorCode)
	{
		return;
	}

	for (std::filesystem::recursive_directory_iterator it(root, errorCode), end; it != end && !errorCode; it.increment(errorCode))
	{
		if (!it->is_directory(errorCode) || errorCode)
		{
			errorCode.clear();
			continue;
		}

		const std::filesystem::path directory = it->path();
		if (!std::filesystem::exists(directory / "compile-request.json", errorCode) || errorCode)
		{
			errorCode.clear();
			continue;
		}

		m_bundles.push_back(ArtifactBundle{BuildBundleLabel(directory), directory});
	}

	std::sort(
	    m_bundles.begin(),
	    m_bundles.end(),
	    [](const ArtifactBundle& lhs, const ArtifactBundle& rhs)
	    {
		    return lhs.Label < rhs.Label;
	    });

	if (!m_bundles.empty())
	{
		SelectBundle(0);
	}
}

void ShaderInspectorPanel::SelectBundle(std::size_t index)
{
	if (index >= m_bundles.size())
	{
		return;
	}

	m_selectedIndex = index;
	m_hasSelection = true;
	LoadSelectedBundleTexts();
}

void ShaderInspectorPanel::LoadSelectedBundleTexts()
{
	if (!m_hasSelection || m_selectedIndex >= m_bundles.size())
	{
		m_texts = {};
		return;
	}

	const std::filesystem::path& directory = m_bundles[m_selectedIndex].Directory;
	m_texts.Source = ReadTextFileOrMessage(directory / "preprocessed-source.hlsl");
	m_texts.Reflection = ReadTextFileOrMessage(directory / "reflection.json");
	m_texts.Disassembly = ReadTextFileOrMessage(directory / "disassembly.txt");
	m_texts.ParameterMatch = ReadTextFileOrMessage(directory / "parameter-struct-match.json");
	m_texts.CompileRequest = ReadTextFileOrMessage(directory / "compile-request.json");
}

void ShaderInspectorPanel::BuildUI(bool disableInteraction)
{
	if (!m_isOpen)
	{
		return;
	}

	if (!m_hasScanned)
	{
		RefreshBundles();
	}

	ImGui::SetNextWindowSize(ImVec2(980.0f, 640.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Shader Inspector", &m_isOpen))
	{
		ImGui::End();
		return;
	}

	ImGui::BeginDisabled(disableInteraction);
	if (ImGui::Button("Refresh"))
	{
		RefreshBundles();
	}
	ImGui::SameLine();
	const std::filesystem::path root = GetArtifactSearchRoot();
	ImGui::TextDisabled("Artifacts: %s", root.generic_string().c_str());
	ImGui::Separator();

	ImGui::BeginChild("##ShaderArtifactList", ImVec2(280.0f, 0.0f), ImGuiChildFlags_Borders);
	if (m_bundles.empty())
	{
		ImGui::TextWrapped("No shader debug artifact bundles found. Run ShaderCompiler cook --debug-artifacts to populate this view.");
	}
	for (std::size_t index = 0; index < m_bundles.size(); ++index)
	{
		const bool selected = m_hasSelection && index == m_selectedIndex;
		if (ImGui::Selectable(m_bundles[index].Label.c_str(), selected))
		{
			SelectBundle(index);
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("##ShaderArtifactDetails", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
	if (!m_hasSelection)
	{
		ImGui::TextDisabled("Select a shader artifact bundle.");
	}
	else
	{
		ImGui::TextUnformatted(m_bundles[m_selectedIndex].Label.c_str());
		ImGui::TextDisabled("%s", m_bundles[m_selectedIndex].Directory.generic_string().c_str());
		ImGui::Separator();
		if (ImGui::BeginTabBar("##ShaderArtifactTabs"))
		{
			if (ImGui::BeginTabItem("Source"))
			{
				DrawTextArtifact("##ShaderSourceText", m_texts.Source);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Reflection"))
			{
				DrawTextArtifact("##ShaderReflectionText", m_texts.Reflection);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Disassembly"))
			{
				DrawTextArtifact("##ShaderDisassemblyText", m_texts.Disassembly);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Param Match"))
			{
				DrawTextArtifact("##ShaderParamMatchText", m_texts.ParameterMatch);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Compile Request"))
			{
				DrawTextArtifact("##ShaderCompileRequestText", m_texts.CompileRequest);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::EndChild();
	ImGui::EndDisabled();
	ImGui::End();
}