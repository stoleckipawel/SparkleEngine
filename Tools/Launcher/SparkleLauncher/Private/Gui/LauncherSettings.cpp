#include "LauncherSettings.h"

namespace SparkleLauncher
{
	LauncherSettings::LauncherSettings(QObject* parent)
	    : QObject(parent)
	{
	}

	const QString& LauncherSettings::EditorProfile() const
	{
		return m_editorProfile;
	}

	const QString& LauncherSettings::RuntimeProfile() const
	{
		return m_runtimeProfile;
	}

	const QString& LauncherSettings::SelectedTargets() const
	{
		return m_selectedTargets;
	}

	const QString& LauncherSettings::ShaderPackages() const
	{
		return m_shaderPackages;
	}

	const QString& LauncherSettings::LaunchBackend() const
	{
		return m_launchBackend;
	}

	const QString& LauncherSettings::LaunchVSync() const
	{
		return m_launchVSync;
	}

	const QString& LauncherSettings::LaunchHighPerformanceAdapter() const
	{
		return m_launchHighPerformanceAdapter;
	}

	const QString& LauncherSettings::LaunchMeshAutoBatching() const
	{
		return m_launchMeshAutoBatching;
	}

	const QString& LauncherSettings::SmokeBackend() const
	{
		return m_smokeBackend;
	}

	const QString& LauncherSettings::SmokeFrameLimit() const
	{
		return m_smokeFrameLimit;
	}

	const QString& LauncherSettings::FormatMode() const
	{
		return m_formatMode;
	}

	const QString& LauncherSettings::CleanScope() const
	{
		return m_cleanScope;
	}

	bool LauncherSettings::ForceConfigure() const
	{
		return m_forceConfigure;
	}

	bool LauncherSettings::ForceRecook() const
	{
		return m_forceRecook;
	}

	bool LauncherSettings::ConfirmForceRecook() const
	{
		return m_confirmForceRecook;
	}

	bool LauncherSettings::ConfirmClean() const
	{
		return m_confirmClean;
	}

	bool LauncherSettings::SmokeTrace() const
	{
		return m_smokeTrace;
	}

	bool LauncherSettings::SmokeSkipLevelSwitching() const
	{
		return m_smokeSkipLevelSwitching;
	}

	void LauncherSettings::SetEditorProfile(const QString& profileName)
	{
		if (m_editorProfile == profileName)
		{
			return;
		}

		m_editorProfile = profileName;
		emit SettingsChanged();
	}

	void LauncherSettings::SetRuntimeProfile(const QString& profileName)
	{
		if (m_runtimeProfile == profileName)
		{
			return;
		}

		m_runtimeProfile = profileName;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSelectedTargets(const QString& targets)
	{
		m_selectedTargets = targets;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderPackages(const QString& packages)
	{
		m_shaderPackages = packages;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchBackend(const QString& backend)
	{
		m_launchBackend = backend;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchVSync(const QString& value)
	{
		m_launchVSync = value;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchHighPerformanceAdapter(const QString& value)
	{
		m_launchHighPerformanceAdapter = value;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchMeshAutoBatching(const QString& value)
	{
		m_launchMeshAutoBatching = value;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSmokeBackend(const QString& backend)
	{
		m_smokeBackend = backend;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSmokeFrameLimit(const QString& frameLimit)
	{
		m_smokeFrameLimit = frameLimit;
		emit SettingsChanged();
	}

	void LauncherSettings::SetFormatMode(const QString& mode)
	{
		m_formatMode = mode;
		emit SettingsChanged();
	}

	void LauncherSettings::SetCleanScope(const QString& scope)
	{
		m_cleanScope = scope;
		emit SettingsChanged();
	}

	void LauncherSettings::SetForceConfigure(bool enabled)
	{
		m_forceConfigure = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetForceRecook(bool enabled)
	{
		m_forceRecook = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetConfirmForceRecook(bool enabled)
	{
		m_confirmForceRecook = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetConfirmClean(bool enabled)
	{
		m_confirmClean = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSmokeTrace(bool enabled)
	{
		m_smokeTrace = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSmokeSkipLevelSwitching(bool enabled)
	{
		m_smokeSkipLevelSwitching = enabled;
		emit SettingsChanged();
	}
}