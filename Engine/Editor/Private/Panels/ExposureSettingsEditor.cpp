#include "PCH.h"

#include "Panels/ExposureSettingsEditor.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <imgui.h>

#include <cfloat>
#include <span>

template <typename TValue> struct ExposureOverrideBinding final
{
	bool& Enabled;
	TValue& Value;
	TValue DefaultValue;
};

struct ExposureFloatPropertyDesc final
{
	const char* Id = nullptr;
	const char* Label = nullptr;
	float Speed = 0.0f;
	float Minimum = 0.0f;
	float Maximum = 0.0f;
	const char* Format = nullptr;
	bool ValueEnabled = true;
};

template <typename TEnum> struct ExposureEnumOption final
{
	const char* Label = nullptr;
	TEnum Value = {};
};

template <typename TEnum> struct ExposureEnumPropertyDesc final
{
	const char* Id = nullptr;
	const char* Label = nullptr;
	std::span<const ExposureEnumOption<TEnum>> Options;
};

class ExposureOverrideTable final
{
public:
	static bool Begin() noexcept
	{
		if (!ImGui::BeginTable("##ViewportExposure", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
		{
			return false;
		}
		ImGui::TableSetupColumn("Override", ImGuiTableColumnFlags_WidthFixed, 28.0f);
		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 175.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		return true;
	}

	static bool DrawFloat(const ExposureFloatPropertyDesc& property, ExposureOverrideBinding<float> binding) noexcept
	{
		bool changed = false;
		ImGui::PushID(property.Id);
		BeginRow(property.Label, binding, changed);
		ImGui::BeginDisabled(!binding.Enabled || !property.ValueEnabled);
		changed |= ImGui::DragFloat("##Value", &binding.Value, property.Speed, property.Minimum, property.Maximum, property.Format);
		ImGui::EndDisabled();
		ImGui::PopID();
		return changed;
	}

	template <typename TEnum>
	static bool DrawEnum(const ExposureEnumPropertyDesc<TEnum>& property, ExposureOverrideBinding<TEnum> binding) noexcept
	{
		bool changed = false;
		ImGui::PushID(property.Id);
		BeginRow(property.Label, binding, changed);
		ImGui::BeginDisabled(!binding.Enabled);
		const char* preview = "Unknown";
		for (const ExposureEnumOption<TEnum>& option : property.Options)
		{
			if (binding.Value == option.Value)
			{
				preview = option.Label;
				break;
			}
		}
		if (ImGui::BeginCombo("##Value", preview))
		{
			for (const ExposureEnumOption<TEnum>& option : property.Options)
			{
				if (ImGui::Selectable(option.Label, binding.Value == option.Value))
				{
					binding.Value = option.Value;
					changed = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();
		ImGui::PopID();
		return changed;
	}

private:
	template <typename TValue> static void BeginRow(const char* label, ExposureOverrideBinding<TValue> binding, bool& changed) noexcept
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		if (ImGui::Checkbox("##Override", &binding.Enabled))
		{
			if (binding.Enabled)
			{
				binding.Value = binding.DefaultValue;
			}
			changed = true;
		}
		ImGui::TableSetColumnIndex(1);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::TableSetColumnIndex(2);
		ImGui::SetNextItemWidth(-FLT_MIN);
	}
};

void ExposureSettingsEditor::DrawSettings(EngineRenderingSettingsSection& settingsSection, const EngineRenderingSettingsState& settings)
{
	using namespace RenderingSettingsPanelUi;

	static constexpr ComboOption<EngineExposureMode> exposureModeOptions[] = {
	    {"Manual", EngineExposureMode::Manual},
	    {"Automatic", EngineExposureMode::Automatic},
	};
	static constexpr ComboOption<EngineExposureMeteringMethod> exposureMeteringMethodOptions[] = {
	    {"Parallel reduction", EngineExposureMeteringMethod::ParallelReduction},
	    {"Downsample pyramid", EngineExposureMeteringMethod::DownsamplePyramid},
	};

	DrawComboOptionRow(
	    "##ExposureMode",
	    "Exposure mode",
	    settings.ExposureMode,
	    exposureModeOptions,
	    [&settingsSection](EngineExposureMode value) { settingsSection.SetExposureMode(value); });
	DrawComboOptionRow(
	    "##ExposureMeteringMethod",
	    "Exposure metering",
	    settings.ExposureMeteringMethod,
	    exposureMeteringMethodOptions,
	    [&settingsSection](EngineExposureMeteringMethod value) { settingsSection.SetExposureMeteringMethod(value); });
	ImGui::BeginDisabled(settings.ExposureMode != EngineExposureMode::Manual);
	DrawFloatInputRow(
	    "##ManualExposure",
	    "Manual exposure",
	    settings.ManualExposure,
	    [&settingsSection](float value) { settingsSection.SetManualExposure(value); },
	    0.1f,
	    1.0f,
	    "%.4f");
	ImGui::EndDisabled();
	DrawFloatInputRow(
	    "##ExposureCompensation",
	    "Exposure compensation EV",
	    settings.ExposureCompensation,
	    [&settingsSection](float value) { settingsSection.SetExposureCompensation(value); },
	    0.1f,
	    1.0f,
	    "%.2f");
	DrawFloatInputRow(
	    "##ExposureTargetLuminance",
	    "Target luminance",
	    settings.ExposureTargetLuminance,
	    [&settingsSection](float value) { settingsSection.SetExposureTargetLuminance(value); },
	    0.01f,
	    0.1f,
	    "%.4f");
	DrawFloatInputRow(
	    "##ExposureMin",
	    "Min exposure",
	    settings.ExposureMin,
	    [&settingsSection](float value) { settingsSection.SetExposureMin(value); },
	    0.0001f,
	    0.01f,
	    "%.6f");
	DrawFloatInputRow(
	    "##ExposureMax",
	    "Max exposure",
	    settings.ExposureMax,
	    [&settingsSection](float value) { settingsSection.SetExposureMax(value); },
	    1.0f,
	    64.0f,
	    "%.3f");
	DrawFloatInputRow(
	    "##ExposureAdaptationSpeedUp",
	    "Adapt speed up",
	    settings.ExposureAdaptationSpeedUp,
	    [&settingsSection](float value) { settingsSection.SetExposureAdaptationSpeedUp(value); },
	    0.1f,
	    1.0f,
	    "%.3f");
	DrawFloatInputRow(
	    "##ExposureAdaptationSpeedDown",
	    "Adapt speed down",
	    settings.ExposureAdaptationSpeedDown,
	    [&settingsSection](float value) { settingsSection.SetExposureAdaptationSpeedDown(value); },
	    0.1f,
	    1.0f,
	    "%.3f");
}

bool ExposureSettingsEditor::DrawOverrides(ViewportExposureOverrides& exposure, const EngineRenderingSettingsState& defaults) noexcept
{
	if (!ExposureOverrideTable::Begin())
	{
		return false;
	}

	static constexpr ExposureEnumOption<EngineExposureMode> exposureModeOptions[] = {
	    {"Manual", EngineExposureMode::Manual},
	    {"Automatic", EngineExposureMode::Automatic},
	};
	static constexpr ExposureEnumOption<EngineExposureMeteringMethod> exposureMeteringOptions[] = {
	    {"Parallel reduction", EngineExposureMeteringMethod::ParallelReduction},
	    {"Downsample pyramid", EngineExposureMeteringMethod::DownsamplePyramid},
	};

	bool changed = false;
	changed |= ExposureOverrideTable::DrawEnum(
	    ExposureEnumPropertyDesc<EngineExposureMode>{
	        .Id = "Mode",
	        .Label = "Mode",
	        .Options = exposureModeOptions,
	    },
	    ExposureOverrideBinding<EngineExposureMode>{
	        .Enabled = exposure.OverrideMode,
	        .Value = exposure.Mode,
	        .DefaultValue = defaults.ExposureMode,
	    });
	changed |= ExposureOverrideTable::DrawEnum(
	    ExposureEnumPropertyDesc<EngineExposureMeteringMethod>{
	        .Id = "Metering",
	        .Label = "Metering",
	        .Options = exposureMeteringOptions,
	    },
	    ExposureOverrideBinding<EngineExposureMeteringMethod>{
	        .Enabled = exposure.OverrideMeteringMethod,
	        .Value = exposure.MeteringMethod,
	        .DefaultValue = defaults.ExposureMeteringMethod,
	    });
	const EngineExposureMode effectiveMode = exposure.OverrideMode ? exposure.Mode : defaults.ExposureMode;
	changed |= ExposureOverrideTable::DrawFloat(
	    ExposureFloatPropertyDesc{
	        .Id = "Manual",
	        .Label = "Manual exposure",
	        .Speed = 0.05f,
	        .Minimum = 0.0f,
	        .Maximum = 65536.0f,
	        .Format = "%.4f",
	        .ValueEnabled = effectiveMode == EngineExposureMode::Manual,
	    },
	    ExposureOverrideBinding<float>{
	        .Enabled = exposure.OverrideManualExposure,
	        .Value = exposure.ManualExposure,
	        .DefaultValue = defaults.ManualExposure,
	    });
	changed |= ExposureOverrideTable::DrawFloat(
	    ExposureFloatPropertyDesc{
	        .Id = "Compensation",
	        .Label = "Compensation (EV)",
	        .Speed = 0.1f,
	        .Minimum = -16.0f,
	        .Maximum = 16.0f,
	        .Format = "%.2f",
	    },
	    ExposureOverrideBinding<float>{
	        .Enabled = exposure.OverrideCompensation,
	        .Value = exposure.Compensation,
	        .DefaultValue = defaults.ExposureCompensation,
	    });
	changed |= ExposureOverrideTable::DrawFloat(
	    ExposureFloatPropertyDesc{
	        .Id = "Target",
	        .Label = "Target luminance",
	        .Speed = 0.01f,
	        .Minimum = 0.0001f,
	        .Maximum = 16.0f,
	        .Format = "%.4f",
	        .ValueEnabled = effectiveMode == EngineExposureMode::Automatic,
	    },
	    ExposureOverrideBinding<float>{
	        .Enabled = exposure.OverrideTargetLuminance,
	        .Value = exposure.TargetLuminance,
	        .DefaultValue = defaults.ExposureTargetLuminance,
	    });
	changed |= ExposureOverrideTable::DrawFloat(
	    ExposureFloatPropertyDesc{
	        .Id = "Minimum",
	        .Label = "Minimum exposure",
	        .Speed = 0.001f,
	        .Minimum = 0.0f,
	        .Maximum = 65536.0f,
	        .Format = "%.6f",
	        .ValueEnabled = effectiveMode == EngineExposureMode::Automatic,
	    },
	    ExposureOverrideBinding<float>{
	        .Enabled = exposure.OverrideMinimum,
	        .Value = exposure.Minimum,
	        .DefaultValue = defaults.ExposureMin,
	    });
	changed |= ExposureOverrideTable::DrawFloat(
	    ExposureFloatPropertyDesc{
	        .Id = "Maximum",
	        .Label = "Maximum exposure",
	        .Speed = 1.0f,
	        .Minimum = 0.0f,
	        .Maximum = 65536.0f,
	        .Format = "%.3f",
	        .ValueEnabled = effectiveMode == EngineExposureMode::Automatic,
	    },
	    ExposureOverrideBinding<float>{
	        .Enabled = exposure.OverrideMaximum,
	        .Value = exposure.Maximum,
	        .DefaultValue = defaults.ExposureMax,
	    });
	changed |= ExposureOverrideTable::DrawFloat(
	    ExposureFloatPropertyDesc{
	        .Id = "AdaptUp",
	        .Label = "Adapt speed up",
	        .Speed = 0.1f,
	        .Minimum = 0.0f,
	        .Maximum = 100.0f,
	        .Format = "%.3f",
	        .ValueEnabled = effectiveMode == EngineExposureMode::Automatic,
	    },
	    ExposureOverrideBinding<float>{
	        .Enabled = exposure.OverrideAdaptationSpeedUp,
	        .Value = exposure.AdaptationSpeedUp,
	        .DefaultValue = defaults.ExposureAdaptationSpeedUp,
	    });
	changed |= ExposureOverrideTable::DrawFloat(
	    ExposureFloatPropertyDesc{
	        .Id = "AdaptDown",
	        .Label = "Adapt speed down",
	        .Speed = 0.1f,
	        .Minimum = 0.0f,
	        .Maximum = 100.0f,
	        .Format = "%.3f",
	        .ValueEnabled = effectiveMode == EngineExposureMode::Automatic,
	    },
	    ExposureOverrideBinding<float>{
	        .Enabled = exposure.OverrideAdaptationSpeedDown,
	        .Value = exposure.AdaptationSpeedDown,
	        .DefaultValue = defaults.ExposureAdaptationSpeedDown,
	    });
	ImGui::EndTable();
	return changed;
}
