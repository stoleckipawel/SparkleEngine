#pragma once

#include "CoreAPI.h"

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ConfigBackedSettings
{
	SPARKLE_CORE_API std::filesystem::path DefaultProjectConfigPath(std::string_view categoryName);
	SPARKLE_CORE_API void LoadSectionValues(
	    const std::filesystem::path& configPath,
	    std::string_view sectionName,
	    const std::function<void(std::string_view key, std::string_view value)>& onValue);
	SPARKLE_CORE_API void WriteSectionValues(
	    const std::filesystem::path& configPath,
	    std::string_view sectionName,
	    const std::vector<std::pair<std::string, std::string>>& values);
}

template <typename TState> class ConfigBackedSettingsSection
{
  public:
	ConfigBackedSettingsSection(std::filesystem::path configPath, std::string_view configSectionName) :
	    m_configPath(std::move(configPath)),
	    m_configSectionName(configSectionName)
	{
	}

	virtual ~ConfigBackedSettingsSection() = default;

	ConfigBackedSettingsSection(const ConfigBackedSettingsSection&) = delete;
	ConfigBackedSettingsSection& operator=(const ConfigBackedSettingsSection&) = delete;
	ConfigBackedSettingsSection(ConfigBackedSettingsSection&&) = delete;
	ConfigBackedSettingsSection& operator=(ConfigBackedSettingsSection&&) = delete;

	const TState& GetState() const noexcept { return m_state; }

	void RefreshFromRuntimeState() noexcept
	{
		m_state = CaptureRuntimeState();
		m_sessionBaseline = m_state;
	}

	void ApplyPersistedValuesToRuntimeState() noexcept
	{
		TState persistedState = CaptureRuntimeState();
		ConfigBackedSettings::LoadSectionValues(
		    m_configPath,
		    m_configSectionName,
		    [this, &persistedState](std::string_view key, std::string_view value)
		    {
			    ReadConfigValue(persistedState, key, value);
		    });
		ApplyStateToRuntime(persistedState);
	}

	bool HasPendingRestart() const noexcept
	{
		return ComputePendingRestart(m_sessionBaseline, m_state);
	}

	std::string BuildPendingRestartMessage() const
	{
		return DescribePendingRestart(m_sessionBaseline, m_state);
	}

  protected:
	void UpdateState(const TState& state)
	{
		m_state = state;
		ApplyStateToRuntime(m_state);
		PersistState(m_state);
	}

  private:
	void PersistState(const TState& state)
	{
		ConfigBackedSettings::WriteSectionValues(m_configPath, m_configSectionName, BuildConfigValues(state));
	}

	virtual TState CaptureRuntimeState() const noexcept = 0;
	virtual void ApplyStateToRuntime(const TState& state) const noexcept = 0;
	virtual void ReadConfigValue(TState& state, std::string_view key, std::string_view value) const = 0;
	virtual std::vector<std::pair<std::string, std::string>> BuildConfigValues(const TState& state) const = 0;
	virtual bool ComputePendingRestart(const TState& baseline, const TState& current) const noexcept = 0;
	virtual std::string DescribePendingRestart(const TState& baseline, const TState& current) const = 0;

	std::filesystem::path m_configPath;
	std::string m_configSectionName;
	TState m_state{};
	TState m_sessionBaseline{};
};
