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

	const QString& LauncherSettings::ShaderBackend() const
	{
		return m_shaderBackend;
	}

	const QString& LauncherSettings::ShaderTargetPreset() const
	{
		return m_shaderTargetPreset;
	}

	const QString& LauncherSettings::ShaderCustomTargets() const
	{
		return m_shaderCustomTargets;
	}

	const QString& LauncherSettings::ShaderCacheDirectory() const
	{
		return m_shaderCacheDirectory;
	}

	const QString& LauncherSettings::ShaderDebugArtifactDirectory() const
	{
		return m_shaderDebugArtifactDirectory;
	}

	const QString& LauncherSettings::LaunchBackend() const
	{
		return m_launchBackend;
	}

	const QString& LauncherSettings::LaunchTarget() const
	{
		return m_launchTarget;
	}

	const QString& LauncherSettings::LaunchStartupLevel() const
	{
		return m_launchStartupLevel;
	}

	const QString& LauncherSettings::LaunchVSync() const
	{
		return m_launchVSync;
	}

	const QString& LauncherSettings::LaunchHighPerformanceAdapter() const
	{
		return m_launchHighPerformanceAdapter;
	}

	const QString& LauncherSettings::LaunchCommandLineArguments() const
	{
		return m_launchCommandLineArguments;
	}

	const QString& LauncherSettings::LaunchCVars() const
	{
		return m_launchCVars;
	}

	const QString& LauncherSettings::CleanScope() const
	{
		return m_cleanScope;
	}

	bool LauncherSettings::ShaderUseCache() const
	{
		return m_shaderUseCache;
	}

	bool LauncherSettings::ShaderEnableDebugInfo() const
	{
		return m_shaderEnableDebugInfo;
	}

	bool LauncherSettings::ShaderEnableOptimizations() const
	{
		return m_shaderEnableOptimizations;
	}

	bool LauncherSettings::ShaderWarningsAsErrors() const
	{
		return m_shaderWarningsAsErrors;
	}

	bool LauncherSettings::ShaderStripDebugInfo() const
	{
		return m_shaderStripDebugInfo;
	}

	bool LauncherSettings::ShaderWriteDebugArtifacts() const
	{
		return m_shaderWriteDebugArtifacts;
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

	void LauncherSettings::SetShaderBackend(const QString& backend)
	{
		if (m_shaderBackend == backend)
		{
			return;
		}
		m_shaderBackend = backend;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderTargetPreset(const QString& preset)
	{
		if (m_shaderTargetPreset == preset)
		{
			return;
		}
		m_shaderTargetPreset = preset;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderCustomTargets(const QString& targets)
	{
		if (m_shaderCustomTargets == targets)
		{
			return;
		}
		m_shaderCustomTargets = targets;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderCacheDirectory(const QString& path)
	{
		if (m_shaderCacheDirectory == path)
		{
			return;
		}
		m_shaderCacheDirectory = path;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderDebugArtifactDirectory(const QString& path)
	{
		if (m_shaderDebugArtifactDirectory == path)
		{
			return;
		}
		m_shaderDebugArtifactDirectory = path;
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

	void LauncherSettings::SetLaunchStartupLevel(const QString& levelName)
	{
		const QString normalized = levelName.trimmed();
		if (m_launchStartupLevel == normalized)
		{
			return;
		}
		m_launchStartupLevel = normalized;
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

	void LauncherSettings::SetCleanScope(const QString& scope)
	{
		if (m_cleanScope == scope)
		{
			return;
		}
		m_cleanScope = scope;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderUseCache(bool enabled)
	{
		if (m_shaderUseCache == enabled)
		{
			return;
		}
		m_shaderUseCache = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderEnableDebugInfo(bool enabled)
	{
		if (m_shaderEnableDebugInfo == enabled)
		{
			return;
		}
		m_shaderEnableDebugInfo = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderEnableOptimizations(bool enabled)
	{
		if (m_shaderEnableOptimizations == enabled)
		{
			return;
		}
		m_shaderEnableOptimizations = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderWarningsAsErrors(bool enabled)
	{
		if (m_shaderWarningsAsErrors == enabled)
		{
			return;
		}
		m_shaderWarningsAsErrors = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderStripDebugInfo(bool enabled)
	{
		if (m_shaderStripDebugInfo == enabled)
		{
			return;
		}
		m_shaderStripDebugInfo = enabled;
		emit SettingsChanged();
	}

	void LauncherSettings::SetShaderWriteDebugArtifacts(bool enabled)
	{
		if (m_shaderWriteDebugArtifacts == enabled)
		{
			return;
		}
		m_shaderWriteDebugArtifacts = enabled;
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

}
