#include "PCH.h"

#include "Panels/RenderingToneMappingSettingsSection.h"

#include "Panels/RenderingSettingsUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsController.h"

#include <imgui.h>

void DrawToneMappingSettingsSection(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings, const char* filterText)
{
	static constexpr RenderingSettingsUi::ComboOption<EngineToneMapper> toneMapperOptions[] = {
	    {"Reinhard", EngineToneMapper::Reinhard},
	    {"ACES approximate", EngineToneMapper::AcesApprox},
	    {"ACES fitted filmic", EngineToneMapper::AcesFilmic},
	};

	if (!RenderingSettingsUi::MatchesFilter(filterText, "Tone Mapping", "tone mapper aces reinhard filmic") || !RenderingSettingsUi::BeginSettingsCategory("Tone Mapping"))
	{
		return;
	}

	if (RenderingSettingsUi::BeginSettingsTable("##RenderingToneMappingSettings"))
	{
		RenderingSettingsUi::DrawComboOptionRow(
		    "##ToneMapper",
		    "Tone mapper",
		    settings.ToneMapper,
		    toneMapperOptions,
		    [&settingsController](EngineToneMapper value) { settingsController.SetToneMapper(value); });

		ImGui::EndTable();
	}
}
