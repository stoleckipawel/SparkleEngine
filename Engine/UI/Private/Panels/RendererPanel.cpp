#include "PCH.h"
#include "RendererPanel.h"

#include <cstdlib>

#include <imgui.h>

static constexpr std::size_t kInvalidSectionIndex = static_cast<std::size_t>(-1);

RendererPanel::RendererPanel(float widthPixels) noexcept : m_widthPixels(widthPixels)
{
	m_sections.reserve(static_cast<std::size_t>(UIRendererSectionId::Count));
}

void RendererPanel::SetWidth(float widthPixels) noexcept
{
	m_widthPixels = widthPixels;
}

std::size_t RendererPanel::FindSectionIndex(UIRendererSectionId id) const noexcept
{
	for (std::size_t i = 0; i < m_sections.size(); ++i)
	{
		if (m_sections[i] && m_sections[i]->GetId() == id)
			return i;
	}

	return kInvalidSectionIndex;
}

void RendererPanel::SetSection(std::unique_ptr<UIRendererSection> section) noexcept
{
	if (!section)
		return;

	const UIRendererSectionId id = section->GetId();
	const std::size_t index = FindSectionIndex(id);
	if (index != kInvalidSectionIndex)
	{
		m_sections[index] = std::move(section);
		return;
	}

	m_sections.emplace_back(std::move(section));
}

bool RendererPanel::HasSection(UIRendererSectionId id) const noexcept
{
	return FindSectionIndex(id) != kInvalidSectionIndex;
}

UIRendererSection& RendererPanel::GetSection(UIRendererSectionId id) noexcept
{
	const std::size_t index = FindSectionIndex(id);
	if (index == kInvalidSectionIndex)
	{
		LOG_FATAL("RendererPanel::GetSection: missing section");
		std::abort();
	}

	return *m_sections[index];
}

void RendererPanel::BuildUI(bool disableInteraction)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - m_widthPixels, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_widthPixels, io.DisplaySize.y), ImGuiCond_Always);

	ImGui::Begin("Renderer", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	for (std::size_t i = 0; i < m_sections.size(); ++i)
	{
		UIRendererSection& section = *m_sections[i];

		ImGui::Text("%s", section.GetTitle());
		ImGui::Separator();
		ImGui::BeginDisabled(disableInteraction);
		section.BuildUI();
		ImGui::EndDisabled();

		if (i + 1 < m_sections.size())
			ImGui::Separator();
	}

	ImGui::End();
}
