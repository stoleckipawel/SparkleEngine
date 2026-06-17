#pragma once

#include "EditorAPI.h"

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace EditorSettingsDetail
{
	SPARKLE_EDITOR_API std::filesystem::path GetEditorSettingsConfigPath();
	SPARKLE_EDITOR_API void LoadConfigSectionValues(
	    const std::filesystem::path& configPath,
	    std::string_view sectionName,
	    const std::function<void(std::string_view key, std::string_view value)>& onValue);
	SPARKLE_EDITOR_API void WriteConfigSectionValues(
	    const std::filesystem::path& configPath,
	    std::string_view sectionName,
	    const std::vector<std::pair<std::string, std::string>>& values);
}

template <typename TState> class EditorConfigBackedSettingsSection
{
  public:
	explicit EditorConfigBackedSettingsSection(std::string_view configSectionName) : m_configSectionName(configSectionName) {}
	virtual ~EditorConfigBackedSettingsSection() = default;

	EditorConfigBackedSettingsSection(const EditorConfigBackedSettingsSection&) = delete;
	EditorConfigBackedSettingsSection& operator=(const EditorConfigBackedSettingsSection&) = delete;
	EditorConfigBackedSettingsSection(EditorConfigBackedSettingsSection&&) = delete;
	EditorConfigBackedSettingsSection& operator=(EditorConfigBackedSettingsSection&&) = delete;

	const TState& GetState() const noexcept { return m_state; }

	void RefreshFromRuntimeState() noexcept
	{
		m_state = CaptureRuntimeState();
		m_sessionBaseline = m_state;
	}

	void ApplyPersistedValuesToRuntimeState() noexcept
	{
		TState persistedState = CaptureRuntimeState();
		EditorSettingsDetail::LoadConfigSectionValues(
		    EditorSettingsDetail::GetEditorSettingsConfigPath(),
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
		EditorSettingsDetail::WriteConfigSectionValues(
		    EditorSettingsDetail::GetEditorSettingsConfigPath(),
		    m_configSectionName,
		    BuildConfigValues(state));
	}

	virtual TState CaptureRuntimeState() const noexcept = 0;
	virtual void ApplyStateToRuntime(const TState& state) const noexcept = 0;
	virtual void ReadConfigValue(TState& state, std::string_view key, std::string_view value) const = 0;
	virtual std::vector<std::pair<std::string, std::string>> BuildConfigValues(const TState& state) const = 0;
	virtual bool ComputePendingRestart(const TState& baseline, const TState& current) const noexcept = 0;
	virtual std::string DescribePendingRestart(const TState& baseline, const TState& current) const = 0;

	std::string m_configSectionName;
	TState m_state{};
	TState m_sessionBaseline{};
};
