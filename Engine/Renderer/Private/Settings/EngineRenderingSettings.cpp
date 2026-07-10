#include "PCH.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include "Core/Public/Console/CVar.h"
#include "Core/Public/Console/CVarRegistry.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Frame/Presentation/OutputEncodingSettings.h"
#include "Frame/Presentation/ToneMappingCVars.h"
#include "Frame/Presentation/ToneMappingSettings.h"
#include "Lighting/LightingCVars.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "Upscaling/UpscalerSettings.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
	constexpr std::string_view kRenderingSettingsSection = "/Script/SparkleRenderer.EngineRenderingSettings";

	std::filesystem::path GetRenderingSettingsConfigPath()
	{
		return Filesystem::GetWorkspaceRootPath() / "Config" / "DefaultEngine.ini";
	}

	template <typename OnValue> void LoadRenderingSettingsConfigValues(OnValue&& onValue)
	{
		std::ifstream input(GetRenderingSettingsConfigPath());
		if (!input.is_open())
		{
			return;
		}

		bool inTargetSection = false;
		for (std::string line; std::getline(input, line);)
		{
			const std::string trimmed = Strings::TrimCopy(line);
			if (trimmed.empty() || trimmed.starts_with(';') || trimmed.starts_with('#'))
			{
				continue;
			}

			if (trimmed.starts_with('[') && trimmed.ends_with(']'))
			{
				inTargetSection = trimmed.size() > 2 && trimmed.substr(1, trimmed.size() - 2) == kRenderingSettingsSection;
				continue;
			}

			if (!inTargetSection)
			{
				continue;
			}

			const std::size_t separator = trimmed.find('=');
			if (separator == std::string::npos)
			{
				continue;
			}

			onValue(std::string_view(trimmed).substr(0, separator), std::string_view(trimmed).substr(separator + 1));
		}
	}

	void WriteRenderingSettingsConfigValues(const std::vector<std::pair<std::string, std::string>>& values)
	{
		const std::filesystem::path configPath = GetRenderingSettingsConfigPath();
		std::error_code errorCode;
		std::filesystem::create_directories(configPath.parent_path(), errorCode);

		std::vector<std::string> lines;
		{
			std::ifstream input(configPath);
			for (std::string line; std::getline(input, line);)
			{
				lines.push_back(std::move(line));
			}
		}

		std::vector<std::string> sectionLines;
		sectionLines.emplace_back("[" + std::string(kRenderingSettingsSection) + "]");
		for (const auto& [key, value] : values)
		{
			sectionLines.emplace_back(key + "=" + value);
		}

		std::size_t sectionStart = lines.size();
		std::size_t sectionEnd = lines.size();
		for (std::size_t index = 0; index < lines.size(); ++index)
		{
			const std::string trimmed = Strings::TrimCopy(lines[index]);
			if (!trimmed.starts_with('[') || !trimmed.ends_with(']'))
			{
				continue;
			}

			if (trimmed.size() > 2 && trimmed.substr(1, trimmed.size() - 2) == kRenderingSettingsSection)
			{
				sectionStart = index;
				sectionEnd = index + 1;
				while (sectionEnd < lines.size())
				{
					const std::string nextTrimmed = Strings::TrimCopy(lines[sectionEnd]);
					if (nextTrimmed.starts_with('[') && nextTrimmed.ends_with(']'))
					{
						break;
					}
					++sectionEnd;
				}
				break;
			}
		}

		if (sectionStart < lines.size())
		{
			lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(sectionStart), lines.begin() + static_cast<std::ptrdiff_t>(sectionEnd));
			lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(sectionStart), sectionLines.begin(), sectionLines.end());
		}
		else
		{
			if (!lines.empty() && !Strings::TrimCopy(lines.back()).empty())
			{
				lines.emplace_back();
			}
			lines.insert(lines.end(), sectionLines.begin(), sectionLines.end());
		}

		std::ofstream output(configPath, std::ios::trunc);
		for (const std::string& line : lines)
		{
			output << line << '\n';
		}
	}

	constexpr std::string_view kPersistedRenderingCVarNames[] = {
	    "r.VSync",
	    "r.BackBufferFormat",
	    "r.PreferHighPerformanceAdapter",
	    "r.ToneMapper",
	    "r.Exposure.Mode",
	    "r.Exposure.MeteringMethod",
	    "r.OutputColorEncoding",
	    "r.Exposure.Manual",
	    "r.Exposure.Compensation",
	    "r.Exposure.TargetLuminance",
	    "r.Exposure.Min",
	    "r.Exposure.Max",
	    "r.Exposure.AdaptationSpeedUp",
	    "r.Exposure.AdaptationSpeedDown",
	    "r.Lighting.MaxDirectionalLights",
	    "r.Lighting.MaxPointLights",
	    "r.Lighting.MaxSpotLights",
	    "r.Lighting.MaxRectLights",
	    "r.MeshAutoBatching",
	    "r.Upscaler.Provider",
	    "r.Upscaler.QualityMode",
	    "r.RayReconstruction.Mode",
	    "r.GBuffer.Mode",
	    "r.Lighting.Mode",
	    "r.RayTracing.Tlas.Refit",
	    "r.RayTracing.PreferPartitionedTlas",
	    "r.RayTracing.Ptlas.PartitionsPerAxis",
	    "r.RayTracing.Ptlas.PartitionUpdateMode",
	    "r.RayTracing.Ptlas.MarkAllDynamicInPartition",
	    "r.RayTracing.Ptlas.ModeChangeDistance"};

	bool IsPersistedRenderingCVarName(std::string_view name) noexcept
	{
		for (const std::string_view persistedName : kPersistedRenderingCVarNames)
		{
			if (name == persistedName)
			{
				return true;
			}
		}
		return false;
	}

	ConsoleVariableBase* FindPersistedRenderingCVar(std::string_view key) noexcept
	{
		const std::string trimmedKey = Strings::TrimCopy(key);
		if (!IsPersistedRenderingCVarName(trimmedKey))
		{
			return nullptr;
		}

		return ConsoleVariableRegistry::Get().Find(trimmedKey);
	}

	void ApplyRenderingSettingsConfigValue(std::string_view key, std::string_view value)
	{
		ConsoleVariableBase* variable = FindPersistedRenderingCVar(key);
		if (variable == nullptr)
		{
			return;
		}

		const std::string trimmedValue = Strings::TrimCopy(value);
		std::string errorMessage;
		(void) variable->TrySetValueFromString(trimmedValue, errorMessage);
	}

	std::vector<std::pair<std::string, std::string>> BuildRenderingSettingsConfigValues()
	{
		std::vector<std::pair<std::string, std::string>> values;
		values.reserve(sizeof(kPersistedRenderingCVarNames) / sizeof(kPersistedRenderingCVarNames[0]));

		const ConsoleVariableRegistry& registry = ConsoleVariableRegistry::Get();
		for (const std::string_view cvarName : kPersistedRenderingCVarNames)
		{
			const ConsoleVariableBase* variable = registry.Find(cvarName);
			if (variable != nullptr)
			{
				values.emplace_back(std::string(variable->GetName()), variable->GetValueAsString());
			}
		}

		return values;
	}

	template <typename TCVar, typename TValue> bool SetCVarIfChanged(TCVar& cvar, const TValue& value) noexcept
	{
		if (cvar.Get() == value)
		{
			return false;
		}

		cvar.Set(value);
		return true;
	}
}

EngineRenderingSettingsSection::EngineRenderingSettingsSection()
{
	RefreshFromRuntimeState();
}

void EngineRenderingSettingsSection::RefreshFromRuntimeState() noexcept
{
	m_state = CaptureRuntimeState();
	m_sessionBaseline = m_state;
}

void EngineRenderingSettingsSection::ApplyPersistedValuesToRuntimeState() noexcept
{
	LoadRenderingSettingsConfigValues(
	    [](std::string_view key, std::string_view value)
	    {
		    ApplyRenderingSettingsConfigValue(key, value);
	    });
	RefreshFromRuntimeState();
}

bool EngineRenderingSettingsSection::HasPendingRestart() const noexcept
{
	return ComputePendingRestart(m_sessionBaseline, m_state);
}

std::string EngineRenderingSettingsSection::BuildPendingRestartMessage() const
{
	return DescribePendingRestart(m_sessionBaseline, m_state);
}

void EngineRenderingSettingsSection::RefreshAndPersistRuntimeState()
{
	m_state = CaptureRuntimeState();
	WriteRenderingSettingsConfigValues(BuildRenderingSettingsConfigValues());
}

void EngineRenderingSettingsSection::SetVSync(bool enabled)
{
	if (SetCVarIfChanged(CVarVSync, enabled))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetBackBufferFormat(PixelFormat format)
{
	if (SetCVarIfChanged(CVarBackBufferFormat, format))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetPreferHighPerformanceAdapter(bool enabled)
{
	if (SetCVarIfChanged(CVarPreferHighPerformanceAdapter, enabled))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetToneMapper(EngineToneMapper toneMapper)
{
	const EngineToneMapper sanitizedToneMapper = SanitizeToneMapper(toneMapper);
	if (SetCVarIfChanged(CVarToneMapper, sanitizedToneMapper))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetExposureMode(EngineExposureMode mode)
{
	const EngineExposureMode sanitizedMode = SanitizeExposureMode(mode);
	if (SetCVarIfChanged(CVarExposureMode, sanitizedMode))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetExposureMeteringMethod(EngineExposureMeteringMethod method)
{
	const EngineExposureMeteringMethod sanitizedMethod = SanitizeExposureMeteringMethod(method);
	if (SetCVarIfChanged(CVarExposureMeteringMethod, sanitizedMethod))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetOutputColorEncoding(EngineOutputColorEncoding encoding)
{
	const EngineOutputColorEncoding sanitizedEncoding = SanitizeOutputColorEncoding(encoding);
	if (SetCVarIfChanged(CVarOutputColorEncoding, sanitizedEncoding))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetManualExposure(float exposure)
{
	const float sanitizedExposure = SanitizeManualExposure(exposure);
	if (SetCVarIfChanged(CVarManualExposure, sanitizedExposure))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetExposureCompensation(float compensation)
{
	const float sanitizedCompensation = SanitizeExposureCompensation(compensation);
	if (SetCVarIfChanged(CVarExposureCompensation, sanitizedCompensation))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetExposureTargetLuminance(float luminance)
{
	const float sanitizedLuminance = SanitizeExposureTargetLuminance(luminance);
	if (SetCVarIfChanged(CVarExposureTargetLuminance, sanitizedLuminance))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetExposureMin(float exposure)
{
	float minExposure = SanitizeExposureMin(exposure);
	float maxExposure = SanitizeExposureMax(CVarExposureMax.Get());
	SanitizeExposureRange(minExposure, maxExposure);

	const bool changedMin = SetCVarIfChanged(CVarExposureMin, minExposure);
	const bool changedMax = SetCVarIfChanged(CVarExposureMax, maxExposure);
	if (changedMin || changedMax)
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetExposureMax(float exposure)
{
	float minExposure = SanitizeExposureMin(CVarExposureMin.Get());
	float maxExposure = SanitizeExposureMax(exposure);
	SanitizeExposureRange(minExposure, maxExposure);

	const bool changedMin = SetCVarIfChanged(CVarExposureMin, minExposure);
	const bool changedMax = SetCVarIfChanged(CVarExposureMax, maxExposure);
	if (changedMin || changedMax)
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetExposureAdaptationSpeedUp(float speed)
{
	const float sanitizedSpeed = SanitizeExposureAdaptationSpeed(speed);
	if (SetCVarIfChanged(CVarExposureAdaptationSpeedUp, sanitizedSpeed))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetExposureAdaptationSpeedDown(float speed)
{
	const float sanitizedSpeed = SanitizeExposureAdaptationSpeed(speed);
	if (SetCVarIfChanged(CVarExposureAdaptationSpeedDown, sanitizedSpeed))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetMaxDirectionalLights(std::uint32_t count)
{
	if (SetCVarIfChanged(CVarMaxDirectionalLights, count))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetMaxPointLights(std::uint32_t count)
{
	if (SetCVarIfChanged(CVarMaxPointLights, count))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetMaxSpotLights(std::uint32_t count)
{
	if (SetCVarIfChanged(CVarMaxSpotLights, count))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetMaxRectLights(std::uint32_t count)
{
	if (SetCVarIfChanged(CVarMaxRectLights, count))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetUpscalerProvider(EUpscalerProviderKind provider)
{
	if (SetCVarIfChanged(CVarUpscalerProvider, provider))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetUpscalerQualityMode(EUpscalerQualityMode mode)
{
	if (SetCVarIfChanged(CVarUpscalerQualityMode, mode))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetRayReconstructionMode(EngineRayReconstructionMode mode)
{
	if (SetCVarIfChanged(CVarRayReconstructionMode, mode))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetGBufferMode(GBufferMode mode)
{
	if (SetCVarIfChanged(CVarGBufferMode, mode))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetLightingMode(LightingMode mode)
{
	if (SetCVarIfChanged(CVarLightingMode, mode))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetMeshAutoBatching(bool enabled)
{
	if (SetCVarIfChanged(CVarRendererMeshAutoBatching, enabled))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetRefitTlas(bool enabled)
{
	if (SetCVarIfChanged(CVarRayTracingClassicTlasRefit, enabled))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetPtlasActive(bool active)
{
	if (SetCVarIfChanged(CVarRayTracingPreferPartitionedTlas, active))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis)
{
	if (SetCVarIfChanged(CVarRayTracingPartitionsPerAxis, partitionsPerAxis))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetPtlasPartitionUpdateMode(RayTracingPtlasPartitionUpdateMode mode)
{
	if (SetCVarIfChanged(CVarRayTracingPtlasPartitionUpdateMode, mode))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetPtlasMarkAllDynamicInPartition(bool enabled)
{
	if (SetCVarIfChanged(CVarRayTracingPtlasMarkAllDynamicInPartition, enabled))
	{
		RefreshAndPersistRuntimeState();
	}
}

void EngineRenderingSettingsSection::SetPtlasModeChangeDistance(float distance)
{
	if (SetCVarIfChanged(CVarRayTracingPtlasModeChangeDistance, distance))
	{
		RefreshAndPersistRuntimeState();
	}
}

EngineRenderingSettingsState EngineRenderingSettingsSection::CaptureRuntimeState() const noexcept
{
	EngineRenderingSettingsState state;
	state.VSync = CVarVSync.Get();
	state.BackBufferFormat = CVarBackBufferFormat.Get();
	state.PreferHighPerformanceAdapter = CVarPreferHighPerformanceAdapter.Get();
	state.ToneMapper = SanitizeToneMapper(CVarToneMapper.Get());
	state.ExposureMode = SanitizeExposureMode(CVarExposureMode.Get());
	state.ExposureMeteringMethod = SanitizeExposureMeteringMethod(CVarExposureMeteringMethod.Get());
	state.OutputColorEncoding = SanitizeOutputColorEncoding(CVarOutputColorEncoding.Get());
	state.ManualExposure = SanitizeManualExposure(CVarManualExposure.Get());
	state.ExposureCompensation = SanitizeExposureCompensation(CVarExposureCompensation.Get());
	state.ExposureTargetLuminance = SanitizeExposureTargetLuminance(CVarExposureTargetLuminance.Get());
	state.ExposureMin = SanitizeExposureMin(CVarExposureMin.Get());
	state.ExposureMax = SanitizeExposureMax(CVarExposureMax.Get());
	SanitizeExposureRange(state.ExposureMin, state.ExposureMax);
	state.ExposureAdaptationSpeedUp = SanitizeExposureAdaptationSpeed(CVarExposureAdaptationSpeedUp.Get());
	state.ExposureAdaptationSpeedDown = SanitizeExposureAdaptationSpeed(CVarExposureAdaptationSpeedDown.Get());
	state.MaxDirectionalLights = CVarMaxDirectionalLights.Get();
	state.MaxPointLights = CVarMaxPointLights.Get();
	state.MaxSpotLights = CVarMaxSpotLights.Get();
	state.MaxRectLights = CVarMaxRectLights.Get();
	state.UpscalerProvider = CVarUpscalerProvider.Get();
	state.UpscalerQualityMode = CVarUpscalerQualityMode.Get();
	state.RayReconstructionMode = CVarRayReconstructionMode.Get();
	state.GBuffer = CVarGBufferMode.Get();
	state.Lighting = GetLightingMode();
	state.MeshAutoBatching = CVarRendererMeshAutoBatching.Get();
	state.RefitTlas = CVarRayTracingClassicTlasRefit.Get();
	state.PtlasActive = CVarRayTracingPreferPartitionedTlas.Get();
	state.PtlasPartitionsPerAxis = CVarRayTracingPartitionsPerAxis.Get();
	state.PtlasPartitionUpdateMode = CVarRayTracingPtlasPartitionUpdateMode.Get();
	state.PtlasMarkAllDynamicInPartition = CVarRayTracingPtlasMarkAllDynamicInPartition.Get();
	state.PtlasModeChangeDistance = CVarRayTracingPtlasModeChangeDistance.Get();
	return state;
}

bool EngineRenderingSettingsSection::ComputePendingRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) const noexcept
{
	return baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter ||
	       baseline.BackBufferFormat != current.BackBufferFormat;
}

std::string EngineRenderingSettingsSection::DescribePendingRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) const
{
	std::vector<std::string> reasons;
	if (baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter)
	{
		reasons.emplace_back("GPU adapter preference");
	}
	if (baseline.BackBufferFormat != current.BackBufferFormat)
	{
		reasons.emplace_back("back buffer format");
	}
	if (reasons.empty())
	{
		return {};
	}

	std::ostringstream stream;
	stream << "Restart the application to apply ";
	for (std::size_t index = 0; index < reasons.size(); ++index)
	{
		if (index > 0)
		{
			stream << (index + 1 == reasons.size() ? " and " : ", ");
		}
		stream << reasons[index];
	}
	stream << ".";
	return stream.str();
}

void ApplyPersistedEngineRenderingSettingsToCVars() noexcept
{
	EngineRenderingSettingsSection renderingSettings;
	renderingSettings.ApplyPersistedValuesToRuntimeState();
}
