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

class EngineRenderingSettingsOperations final
{
  public:
	static constexpr std::string_view kRenderingSettingsSection = "/Script/SparkleRenderer.EngineRenderingSettings";

	static std::filesystem::path GetRenderingSettingsConfigPath()
	{
		return Filesystem::GetWorkspaceRootPath() / "Config" / "DefaultEngine.ini";
	}

	template <typename OnValue> static void LoadRenderingSettingsConfigValues(OnValue&& onValue)
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

	static void WriteRenderingSettingsConfigValues(const std::vector<std::pair<std::string, std::string>>& values)
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

	static constexpr std::string_view kPersistedRenderingCVarNames[] = {
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

	static bool IsPersistedRenderingCVarName(std::string_view name) noexcept
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

	static ConsoleVariableBase* FindPersistedRenderingCVar(std::string_view key) noexcept
	{
		const std::string trimmedKey = Strings::TrimCopy(key);
		if (!IsPersistedRenderingCVarName(trimmedKey))
		{
			return nullptr;
		}

		return ConsoleVariableRegistry::Get().Find(trimmedKey);
	}

	static void ApplyRenderingSettingsConfigValue(std::string_view key, std::string_view value)
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

	template <typename TValue>
	static std::string PersistedValue(TValue value)
	{
		if constexpr (std::is_enum_v<TValue>)
		{
			return std::to_string(static_cast<std::underlying_type_t<TValue>>(value));
		}
		else if constexpr (std::is_same_v<TValue, bool>)
		{
			return value ? "1" : "0";
		}
		else
		{
			return std::to_string(value);
		}
	}

	static std::vector<std::pair<std::string, std::string>> BuildRenderingSettingsConfigValues(
	    const EngineRenderingSettingsState& state)
	{
		std::vector<std::pair<std::string, std::string>> values;
		values.reserve(sizeof(kPersistedRenderingCVarNames) / sizeof(kPersistedRenderingCVarNames[0]));
		values.emplace_back("r.VSync", PersistedValue(state.VSync));
		values.emplace_back("r.BackBufferFormat", PersistedValue(state.BackBufferFormat));
		values.emplace_back("r.PreferHighPerformanceAdapter", PersistedValue(state.PreferHighPerformanceAdapter));
		values.emplace_back("r.ToneMapper", PersistedValue(state.ToneMapper));
		values.emplace_back("r.Exposure.Mode", PersistedValue(state.ExposureMode));
		values.emplace_back("r.Exposure.MeteringMethod", PersistedValue(state.ExposureMeteringMethod));
		values.emplace_back("r.OutputColorEncoding", PersistedValue(state.OutputColorEncoding));
		values.emplace_back("r.Exposure.Manual", PersistedValue(state.ManualExposure));
		values.emplace_back("r.Exposure.Compensation", PersistedValue(state.ExposureCompensation));
		values.emplace_back("r.Exposure.TargetLuminance", PersistedValue(state.ExposureTargetLuminance));
		values.emplace_back("r.Exposure.Min", PersistedValue(state.ExposureMin));
		values.emplace_back("r.Exposure.Max", PersistedValue(state.ExposureMax));
		values.emplace_back("r.Exposure.AdaptationSpeedUp", PersistedValue(state.ExposureAdaptationSpeedUp));
		values.emplace_back("r.Exposure.AdaptationSpeedDown", PersistedValue(state.ExposureAdaptationSpeedDown));
		values.emplace_back("r.Lighting.MaxDirectionalLights", PersistedValue(state.MaxDirectionalLights));
		values.emplace_back("r.Lighting.MaxPointLights", PersistedValue(state.MaxPointLights));
		values.emplace_back("r.Lighting.MaxSpotLights", PersistedValue(state.MaxSpotLights));
		values.emplace_back("r.Lighting.MaxRectLights", PersistedValue(state.MaxRectLights));
		values.emplace_back("r.MeshAutoBatching", PersistedValue(state.MeshAutoBatching));
		values.emplace_back("r.Upscaler.Provider", PersistedValue(state.UpscalerProvider));
		values.emplace_back("r.Upscaler.QualityMode", PersistedValue(state.UpscalerQualityMode));
		values.emplace_back("r.RayReconstruction.Mode", PersistedValue(state.RayReconstructionMode));
		values.emplace_back("r.GBuffer.Mode", PersistedValue(state.GBuffer));
		values.emplace_back("r.Lighting.Mode", PersistedValue(state.Lighting));
		values.emplace_back("r.RayTracing.Tlas.Refit", PersistedValue(state.RefitTlas));
		values.emplace_back("r.RayTracing.PreferPartitionedTlas", PersistedValue(state.PtlasActive));
		values.emplace_back("r.RayTracing.Ptlas.PartitionsPerAxis", PersistedValue(state.PtlasPartitionsPerAxis));
		values.emplace_back("r.RayTracing.Ptlas.PartitionUpdateMode", PersistedValue(state.PtlasPartitionUpdateMode));
		values.emplace_back("r.RayTracing.Ptlas.MarkAllDynamicInPartition", PersistedValue(state.PtlasMarkAllDynamicInPartition));
		values.emplace_back("r.RayTracing.Ptlas.ModeChangeDistance", PersistedValue(state.PtlasModeChangeDistance));
		return values;
	}

	template <typename TCVar, typename TValue> static bool SetCVarIfChanged(TCVar& cvar, const TValue& value) noexcept
	{
		if (cvar.Get() == value)
		{
			return false;
		}

		cvar.Set(value);
		return true;
	}
};

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
	EngineRenderingSettingsOperations::LoadRenderingSettingsConfigValues(
	    [](std::string_view key, std::string_view value)
	    {
		    EngineRenderingSettingsOperations::ApplyRenderingSettingsConfigValue(key, value);
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

void EngineRenderingSettingsSection::SetCommitHandler(CommitHandler handler)
{
	m_commitHandler = std::move(handler);
}

void EngineRenderingSettingsSection::CommitState()
{
	EngineRenderingSettingsOperations::WriteRenderingSettingsConfigValues(
	    EngineRenderingSettingsOperations::BuildRenderingSettingsConfigValues(m_state));
	if (m_commitHandler)
	{
		m_commitHandler(m_state);
		return;
	}
	ApplyEngineRenderingSettingsStateToCVars(m_state);
}

void EngineRenderingSettingsSection::SetVSync(bool enabled)
{
	SetValue(m_state.VSync, enabled);
}

void EngineRenderingSettingsSection::SetBackBufferFormat(PixelFormat format)
{
	SetValue(m_state.BackBufferFormat, format);
}

void EngineRenderingSettingsSection::SetPreferHighPerformanceAdapter(bool enabled)
{
	SetValue(m_state.PreferHighPerformanceAdapter, enabled);
}

void EngineRenderingSettingsSection::SetToneMapper(EngineToneMapper toneMapper)
{
	SetValue(m_state.ToneMapper, SanitizeToneMapper(toneMapper));
}

void EngineRenderingSettingsSection::SetExposureMode(EngineExposureMode mode)
{
	SetValue(m_state.ExposureMode, SanitizeExposureMode(mode));
}

void EngineRenderingSettingsSection::SetExposureMeteringMethod(EngineExposureMeteringMethod method)
{
	SetValue(m_state.ExposureMeteringMethod, SanitizeExposureMeteringMethod(method));
}

void EngineRenderingSettingsSection::SetOutputColorEncoding(EngineOutputColorEncoding encoding)
{
	SetValue(m_state.OutputColorEncoding, SanitizeOutputColorEncoding(encoding));
}

void EngineRenderingSettingsSection::SetManualExposure(float exposure)
{
	SetValue(m_state.ManualExposure, SanitizeManualExposure(exposure));
}

void EngineRenderingSettingsSection::SetExposureCompensation(float compensation)
{
	SetValue(m_state.ExposureCompensation, SanitizeExposureCompensation(compensation));
}

void EngineRenderingSettingsSection::SetExposureTargetLuminance(float luminance)
{
	SetValue(m_state.ExposureTargetLuminance, SanitizeExposureTargetLuminance(luminance));
}

void EngineRenderingSettingsSection::SetExposureMin(float exposure)
{
	float minExposure = SanitizeExposureMin(exposure);
	float maxExposure = SanitizeExposureMax(m_state.ExposureMax);
	SanitizeExposureRange(minExposure, maxExposure);
	if (m_state.ExposureMin != minExposure || m_state.ExposureMax != maxExposure)
	{
		m_state.ExposureMin = minExposure;
		m_state.ExposureMax = maxExposure;
		CommitState();
	}
}

void EngineRenderingSettingsSection::SetExposureMax(float exposure)
{
	float minExposure = SanitizeExposureMin(m_state.ExposureMin);
	float maxExposure = SanitizeExposureMax(exposure);
	SanitizeExposureRange(minExposure, maxExposure);
	if (m_state.ExposureMin != minExposure || m_state.ExposureMax != maxExposure)
	{
		m_state.ExposureMin = minExposure;
		m_state.ExposureMax = maxExposure;
		CommitState();
	}
}

void EngineRenderingSettingsSection::SetExposureAdaptationSpeedUp(float speed)
{
	SetValue(m_state.ExposureAdaptationSpeedUp, SanitizeExposureAdaptationSpeed(speed));
}

void EngineRenderingSettingsSection::SetExposureAdaptationSpeedDown(float speed)
{
	SetValue(m_state.ExposureAdaptationSpeedDown, SanitizeExposureAdaptationSpeed(speed));
}

void EngineRenderingSettingsSection::SetMaxDirectionalLights(std::uint32_t count)
{
	SetValue(m_state.MaxDirectionalLights, count);
}

void EngineRenderingSettingsSection::SetMaxPointLights(std::uint32_t count)
{
	SetValue(m_state.MaxPointLights, count);
}

void EngineRenderingSettingsSection::SetMaxSpotLights(std::uint32_t count)
{
	SetValue(m_state.MaxSpotLights, count);
}

void EngineRenderingSettingsSection::SetMaxRectLights(std::uint32_t count)
{
	SetValue(m_state.MaxRectLights, count);
}

void EngineRenderingSettingsSection::SetUpscalerProvider(EUpscalerProviderKind provider)
{
	SetValue(m_state.UpscalerProvider, provider);
}

void EngineRenderingSettingsSection::SetUpscalerQualityMode(EUpscalerQualityMode mode)
{
	SetValue(m_state.UpscalerQualityMode, mode);
}

void EngineRenderingSettingsSection::SetRayReconstructionMode(EngineRayReconstructionMode mode)
{
	SetValue(m_state.RayReconstructionMode, mode);
}

void EngineRenderingSettingsSection::SetGBufferMode(GBufferMode mode)
{
	SetValue(m_state.GBuffer, mode);
}

void EngineRenderingSettingsSection::SetLightingMode(LightingMode mode)
{
	SetValue(m_state.Lighting, mode);
}

void EngineRenderingSettingsSection::SetMeshAutoBatching(bool enabled)
{
	SetValue(m_state.MeshAutoBatching, enabled);
}

void EngineRenderingSettingsSection::SetRefitTlas(bool enabled)
{
	SetValue(m_state.RefitTlas, enabled);
}

void EngineRenderingSettingsSection::SetPtlasActive(bool active)
{
	SetValue(m_state.PtlasActive, active);
}

void EngineRenderingSettingsSection::SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis)
{
	SetValue(m_state.PtlasPartitionsPerAxis, partitionsPerAxis);
}

void EngineRenderingSettingsSection::SetPtlasPartitionUpdateMode(RayTracingPtlasPartitionUpdateMode mode)
{
	SetValue(m_state.PtlasPartitionUpdateMode, mode);
}

void EngineRenderingSettingsSection::SetPtlasMarkAllDynamicInPartition(bool enabled)
{
	SetValue(m_state.PtlasMarkAllDynamicInPartition, enabled);
}

void EngineRenderingSettingsSection::SetPtlasModeChangeDistance(float distance)
{
	SetValue(m_state.PtlasModeChangeDistance, distance);
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

void ApplyEngineRenderingSettingsStateToCVars(
    const EngineRenderingSettingsState& state) noexcept
{
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarVSync, state.VSync);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarBackBufferFormat, state.BackBufferFormat);
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarPreferHighPerformanceAdapter,
	    state.PreferHighPerformanceAdapter);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarToneMapper, SanitizeToneMapper(state.ToneMapper));
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarExposureMode, SanitizeExposureMode(state.ExposureMode));
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarExposureMeteringMethod,
	    SanitizeExposureMeteringMethod(state.ExposureMeteringMethod));
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarOutputColorEncoding,
	    SanitizeOutputColorEncoding(state.OutputColorEncoding));
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarManualExposure,
	    SanitizeManualExposure(state.ManualExposure));
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarExposureCompensation,
	    SanitizeExposureCompensation(state.ExposureCompensation));
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarExposureTargetLuminance,
	    SanitizeExposureTargetLuminance(state.ExposureTargetLuminance));
	float minimumExposure = SanitizeExposureMin(state.ExposureMin);
	float maximumExposure = SanitizeExposureMax(state.ExposureMax);
	SanitizeExposureRange(minimumExposure, maximumExposure);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarExposureMin, minimumExposure);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarExposureMax, maximumExposure);
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarExposureAdaptationSpeedUp,
	    SanitizeExposureAdaptationSpeed(state.ExposureAdaptationSpeedUp));
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarExposureAdaptationSpeedDown,
	    SanitizeExposureAdaptationSpeed(state.ExposureAdaptationSpeedDown));
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarMaxDirectionalLights, state.MaxDirectionalLights);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarMaxPointLights, state.MaxPointLights);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarMaxSpotLights, state.MaxSpotLights);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarMaxRectLights, state.MaxRectLights);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarUpscalerProvider, state.UpscalerProvider);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarUpscalerQualityMode, state.UpscalerQualityMode);
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarRayReconstructionMode,
	    state.RayReconstructionMode);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarGBufferMode, state.GBuffer);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarLightingMode, state.Lighting);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarRendererMeshAutoBatching, state.MeshAutoBatching);
	EngineRenderingSettingsOperations::SetCVarIfChanged(CVarRayTracingClassicTlasRefit, state.RefitTlas);
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarRayTracingPreferPartitionedTlas,
	    state.PtlasActive);
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarRayTracingPartitionsPerAxis,
	    state.PtlasPartitionsPerAxis);
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarRayTracingPtlasPartitionUpdateMode,
	    state.PtlasPartitionUpdateMode);
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarRayTracingPtlasMarkAllDynamicInPartition,
	    state.PtlasMarkAllDynamicInPartition);
	EngineRenderingSettingsOperations::SetCVarIfChanged(
	    CVarRayTracingPtlasModeChangeDistance,
	    state.PtlasModeChangeDistance);
}

void ApplyPersistedEngineRenderingSettingsToCVars() noexcept
{
	EngineRenderingSettingsSection renderingSettings;
	renderingSettings.ApplyPersistedValuesToRuntimeState();
}
