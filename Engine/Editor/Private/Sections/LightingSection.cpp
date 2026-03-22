#include "PCH.h"
#include "Sections/LightingSection.h"

#include "Level/Level.h"
#include "Runtime/Level/LevelManager.h"
#include "Scene/Lighting/GameDirectionalLight.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <string>
#include <vector>

#include <imgui.h>

void LightingSection::ClampLightingUiValues(DirectX::XMFLOAT3& color, float& intensity) noexcept
{
	color.x = std::clamp(color.x, 0.0f, 1.0f);
	color.y = std::clamp(color.y, 0.0f, 1.0f);
	color.z = std::clamp(color.z, 0.0f, 1.0f);
	intensity = (std::max)(0.0f, intensity);
}

LightingSection::LightingSection(LevelManager& levelManager, SceneLighting& sceneLighting) noexcept :
	m_levelManager(&levelManager), m_sceneLighting(&sceneLighting)
{
}

std::vector<LightingSection::LightSelectionEntry> LightingSection::BuildSelectionEntries() const
{
	std::vector<LightSelectionEntry> entries;
	if (m_levelManager == nullptr)
	{
		return entries;
	}

	const LevelAsset* activeLevel = m_levelManager->GetActiveLevel();
	if (activeLevel == nullptr)
	{
		return entries;
	}

	const LevelDesc& levelDesc = activeLevel->GetLevelDesc();
	const std::uint32_t directionalLightCount = levelDesc.lightingDesc.directionalLightCount;

	entries.reserve(static_cast<std::size_t>(directionalLightCount));

	for (std::size_t lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
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

	if (m_sceneLighting == nullptr)
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

	GameDirectionalLight& light = m_sceneLighting->GetGameDirectionalLight(directionalLightIndex);
	DirectionalLightDesc lightDesc = light.GetDesc();
	DirectX::XMFLOAT3 direction = lightDesc.direction;
	float directionValues[3] = {direction.x, direction.y, direction.z};
	ImGui::PushID("DirectionalDirection");
	if (UiUtil::EditFloat3SliderWithInput("Direction", directionValues, kDirectionSliderMin, kDirectionSliderMax, "%.2f", "%.3f"))
	{
		lightDesc.direction = {directionValues[0], directionValues[1], directionValues[2]};
		light.ApplyDesc(lightDesc);
	}
	ImGui::PopID();

	float intensity = lightDesc.intensity;
	ImGui::PushID("DirectionalIntensity");
	if (UiUtil::EditFloatSliderWithInput("Intensity", intensity, kIntensitySliderMin, kIntensitySliderMax, "%.2f", "%.3f"))
	{
		DirectX::XMFLOAT3 dummyColor = {1.0f, 1.0f, 1.0f};
		ClampLightingUiValues(dummyColor, intensity);
		lightDesc.intensity = intensity;
		light.ApplyDesc(lightDesc);
	}
	ImGui::PopID();

	DirectX::XMFLOAT3 color = lightDesc.color;
	float colorValues[3] = {color.x, color.y, color.z};
	ImGui::PushID("DirectionalColor");
	if (UiUtil::EditColor3("Color", colorValues))
	{
		DirectX::XMFLOAT3 clampedColor = {colorValues[0], colorValues[1], colorValues[2]};
		float dummyIntensity = 1.0f;
		ClampLightingUiValues(clampedColor, dummyIntensity);
		lightDesc.color = clampedColor;
		light.ApplyDesc(lightDesc);
	}
	ImGui::PopID();
}