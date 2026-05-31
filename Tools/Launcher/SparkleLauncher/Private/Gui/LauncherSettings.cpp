#include "LauncherSettings.h"

namespace SparkleLauncher
{
	static QString NormalizeBuildConfiguration(const QString& configuration)
	{
		const QString lowered = configuration.trimmed().toLower();
		if (lowered == "debug" || lowered.startsWith("debug"))
		{
			return "debug";
		}
		if (lowered == "shipping" || lowered.startsWith("shipping"))
		{
			return "shipping";
		}
		return "development";
	}

	LauncherSettings::LauncherSettings(QObject* parent)
	    : QObject(parent)
	{
	}

	const QString& LauncherSettings::BuildConfiguration() const
	{
		return m_buildConfiguration;
	}

	const QString& LauncherSettings::EditorProfile() const
	{
		static QString debugProfile = "DebugEditor";
		static QString developmentProfile = "DevelopmentEditor";
		static QString shippingProfile = "ShippingEditor";
		if (m_buildConfiguration == "debug")
		{
			return debugProfile;
		}
		if (m_buildConfiguration == "shipping")
		{
			return shippingProfile;
		}
		return developmentProfile;
	}

	const QString& LauncherSettings::RuntimeProfile() const
	{
		static QString debugProfile = "DebugGame";
		static QString developmentProfile = "DevelopmentGame";
		static QString shippingProfile = "ShippingGame";
		if (m_buildConfiguration == "debug")
		{
			return debugProfile;
		}
		if (m_buildConfiguration == "shipping")
		{
			return shippingProfile;
		}
		return developmentProfile;
	}

	const QString& LauncherSettings::WorkspaceIde() const
	{
		return m_workspaceIde;
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

	const QString& LauncherSettings::LaunchTarget() const
	{
		return m_launchTarget;
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

	const QString& LauncherSettings::LaunchCommandLineArguments() const
	{
		return m_launchCommandLineArguments;
	}

	const QString& LauncherSettings::LaunchCVars() const
	{
		return m_launchCVars;
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

	bool LauncherSettings::LaunchSmokeTest() const
	{
		return m_launchSmokeTest;
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

	void LauncherSettings::SetBuildConfiguration(const QString& configuration)
	{
		const QString normalized = NormalizeBuildConfiguration(configuration);
		if (m_buildConfiguration == normalized)
		{
			return;
		}

		m_buildConfiguration = normalized;
		emit SettingsChanged();
	}

	void LauncherSettings::SetEditorProfile(const QString& profileName)
	{
		SetBuildConfiguration(profileName);
	}

	void LauncherSettings::SetRuntimeProfile(const QString& profileName)
	{
		SetBuildConfiguration(profileName);
	}

	void LauncherSettings::SetWorkspaceIde(const QString& ide)
	{
		if (m_workspaceIde == ide)
		{
			return;
		}

		m_workspaceIde = ide;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSelectedTargets(const QString& targets)
	{
		if (m_selectedTargets == targets)
		{
			return;
		}
		m_selectedTargets = targets;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderPackages(const QString& packages)
	{
		if (m_shaderPackages == packages)
		{
			return;
		}
		m_shaderPackages = packages;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchBackend(const QString& backend)
	{
		if (m_launchBackend == backend)
		{
			return;
		}
		m_launchBackend = backend;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchTarget(const QString& target)
	{
		if (m_launchTarget == target)
		{
			return;
		}
		m_launchTarget = target;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchVSync(const QString& value)
	{
		if (m_launchVSync == value)
		{
			return;
		}
		m_launchVSync = value;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchHighPerformanceAdapter(const QString& value)
	{
		if (m_launchHighPerformanceAdapter == value)
		{
			return;
		}
		m_launchHighPerformanceAdapter = value;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchMeshAutoBatching(const QString& value)
	{
		if (m_launchMeshAutoBatching == value)
		{
			return;
		}
		m_launchMeshAutoBatching = value;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchCommandLineArguments(const QString& arguments)
	{
		if (m_launchCommandLineArguments == arguments)
		{
			return;
		}
		m_launchCommandLineArguments = arguments;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchCVars(const QString& cvars)
	{
		if (m_launchCVars == cvars)
		{
			return;
		}
		m_launchCVars = cvars;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSmokeBackend(const QString& backend)
	{
		if (m_smokeBackend == backend)
		{
			return;
		}
		m_smokeBackend = backend;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSmokeFrameLimit(const QString& frameLimit)
	{
		if (m_smokeFrameLimit == frameLimit)
		{
			return;
		}
		m_smokeFrameLimit = frameLimit;
		emit SettingsChanged();
	}

	void LauncherSettings::SetFormatMode(const QString& mode)
	{
		if (m_formatMode == mode)
		{
			return;
		}
		m_formatMode = mode;
		emit SettingsChanged();
	}

	void LauncherSettings::SetCleanScope(const QString& scope)
	{
		if (m_cleanScope == scope)
		{
			return;
		}
		m_cleanScope = scope;
		emit SettingsChanged();
	}

	void LauncherSettings::SetLaunchSmokeTest(bool enabled)
	{
		if (m_launchSmokeTest == enabled)
		{
			return;
		}
		m_launchSmokeTest = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetForceConfigure(bool enabled)
	{
		if (m_forceConfigure == enabled)
		{
			return;
		}
		m_forceConfigure = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetForceRecook(bool enabled)
	{
		if (m_forceRecook == enabled)
		{
			return;
		}
		m_forceRecook = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetConfirmForceRecook(bool enabled)
	{
		if (m_confirmForceRecook == enabled)
		{
			return;
		}
		m_confirmForceRecook = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetConfirmClean(bool enabled)
	{
		if (m_confirmClean == enabled)
		{
			return;
		}
		m_confirmClean = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSmokeTrace(bool enabled)
	{
		if (m_smokeTrace == enabled)
		{
			return;
		}
		m_smokeTrace = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetSmokeSkipLevelSwitching(bool enabled)
	{
		if (m_smokeSkipLevelSwitching == enabled)
		{
			return;
		}
		m_smokeSkipLevelSwitching = enabled;
		emit SettingsChanged();
	}
}
