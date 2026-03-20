#include "PCH.h"
#include "Sections/LightingSection.h"

#include "Level/Level.h"
#include "Runtime/Level/LevelManager.h"
#include "Scene/Lighting/DirectionalLightDesc.h"
#include "Scene/Lighting/GameSceneLightingState.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <string>
#include <vector>

#include <imgui.h>

namespace
{
	constexpr float DirectionSliderMin = -1.0f;
	constexpr float DirectionSliderMax = 1.0f;
	constexpr float IntensitySliderMin = 0.0f;
	constexpr float IntensitySliderMax = 20.0f;

	void ClampLightingUiValues(DirectX::XMFLOAT3& color, float& intensity)
	{
		color.x = std::clamp(color.x, 0.0f, 1.0f);
		color.y = std::clamp(color.y, 0.0f, 1.0f);
		color.z = std::clamp(color.z, 0.0f, 1.0f);
		intensity = (std::max) (0.0f, intensity);
	}
}  // namespace

LightingSection::LightingSection(LevelManager& levelManager) noexcept : m_levelManager(&levelManager) {}

std::vector<LightingSection::LightSelectionEntry> LightingSection::BuildSelectionEntries() const
{
	std::vector<LightSelectionEntry> entries;
	if (m_levelManager == nullptr)
	{
		return entries;
	}

	const Level* activeLevel = m_levelManager->GetActiveLevel();
	if (activeLevel == nullptr)
	{
		return entries;
	}

	entries.reserve(static_cast<std::size_t>(activeLevel->GetDirectionalLightCount()));

	for (std::size_t lightIndex = 0; lightIndex < activeLevel->GetDirectionalLightCount(); ++lightIndex)
	{
		entries.push_back({lightIndex, "Directional Light " + std::to_string(lightIndex + 1)});
	}

	return entries;
}

void LightingSection::BuildUI()
{
	if (m_levelManager == nullptr)
	{
		ImGui::TextDisabled("Lighting controls unavailable");
		return;
	}

	GameSceneLightingState* lightingState = m_levelManager->GetGameSceneLightingState();
	if (lightingState == nullptr)
	{
		ImGui::TextDisabled("Scene lighting unavailable");
		return;
	}

	const std::vector<LightSelectionEntry> selectionEntries = BuildSelectionEntries();
	if (selectionEntries.empty())
	{
		ImGui::TextDisabled("No lights are defined in the active level");
		return;
	}

	const bool hasActiveLevel = m_levelManager->HasActiveLevel();
	const int maxSelectionIndex = static_cast<int>(selectionEntries.size()) - 1;
	m_selectedLightIndex = std::clamp(m_selectedLightIndex, 0, maxSelectionIndex);

	std::vector<const char*> selectionLabels;
	selectionLabels.reserve(selectionEntries.size());
	for (const LightSelectionEntry& entry : selectionEntries)
	{
		selectionLabels.push_back(entry.label.c_str());
	}

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::Combo("##SelectedLight", &m_selectedLightIndex, selectionLabels.data(), static_cast<int>(selectionLabels.size()));

	const LightSelectionEntry& selectedEntry = selectionEntries[static_cast<std::size_t>(m_selectedLightIndex)];
	const std::size_t directionalLightIndex = selectedEntry.lightIndex;

	DirectionalLightDesc directionalLight = lightingState->GetDirectionalLight(directionalLightIndex);
	DirectX::XMFLOAT3 direction = directionalLight.direction;
	float directionValues[3] = {direction.x, direction.y, direction.z};
	ImGui::PushID("DirectionalDirection");
	if (UiUtil::EditFloat3SliderWithInput("Direction", directionValues, DirectionSliderMin, DirectionSliderMax, "%.2f", "%.3f"))
	{
		directionalLight.direction = {directionValues[0], directionValues[1], directionValues[2]};
		lightingState->SetDirectionalLight(directionalLightIndex, directionalLight);
	}
	ImGui::PopID();

	float intensity = directionalLight.intensity;
	ImGui::PushID("DirectionalIntensity");
	if (UiUtil::EditFloatSliderWithInput("Intensity", intensity, IntensitySliderMin, IntensitySliderMax, "%.2f", "%.3f"))
	{
		DirectX::XMFLOAT3 dummyColor = {1.0f, 1.0f, 1.0f};
		ClampLightingUiValues(dummyColor, intensity);
		directionalLight.intensity = intensity;
		lightingState->SetDirectionalLight(directionalLightIndex, directionalLight);
	}
	ImGui::PopID();

	DirectX::XMFLOAT3 color = directionalLight.color;
	float colorValues[3] = {color.x, color.y, color.z};
	ImGui::PushID("DirectionalColor");
	if (UiUtil::EditColor3("Color", colorValues))
	{
		DirectX::XMFLOAT3 clampedColor = {colorValues[0], colorValues[1], colorValues[2]};
		float dummyIntensity = 1.0f;
		ClampLightingUiValues(clampedColor, dummyIntensity);
		directionalLight.color = clampedColor;
		lightingState->SetDirectionalLight(directionalLightIndex, directionalLight);
	}
	ImGui::PopID();

	if (!m_statusMessage.empty())
	{
		const ImVec4 colorValue = m_bLastSaveSucceeded ? ImVec4(0.3f, 0.8f, 0.4f, 1.0f) : ImVec4(0.9f, 0.4f, 0.3f, 1.0f);
		ImGui::TextColored(colorValue, "%s", m_statusMessage.c_str());
	}

	if (!hasActiveLevel)
	{
		ImGui::BeginDisabled();
	}

	const float buttonWidth = ImGui::GetContentRegionAvail().x;
	if (ImGui::Button("Save Defaults", ImVec2(buttonWidth, 0.0f)))
	{
		m_bLastSaveSucceeded = m_levelManager->SaveActiveLevelLightingDefaults();
		m_statusMessage = m_bLastSaveSucceeded ? "Saved lighting defaults" : "Failed to save lighting defaults";
	}

	if (!hasActiveLevel)
	{
		ImGui::EndDisabled();
	}
}