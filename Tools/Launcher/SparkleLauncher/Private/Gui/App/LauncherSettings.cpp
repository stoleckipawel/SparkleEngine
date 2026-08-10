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

	LauncherSettings::LauncherSettings(QObject* parent) :
	    QObject(parent)
	{
	}

	const QString& LauncherSettings::RunMode() const
	{
		return m_runMode;
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

	const QString& LauncherSettings::WorkspaceCompiler() const
	{
		return m_workspaceCompiler;
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

	const QString& LauncherSettings::GraphicsApi() const
	{
		return m_graphicsApi;
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

	void LauncherSettings::SetRunMode(const QString& runMode)
	{
		const QString normalized = runMode.trimmed().toLower() == "game" ? QStringLiteral("game") : QStringLiteral("editor");
		if (m_runMode == normalized)
		{
			return;
		}

		m_runMode = normalized;
		emit SettingsChanged();
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

	void LauncherSettings::SetWorkspaceCompiler(const QString& compiler)
	{
		if (m_workspaceCompiler == compiler)
		{
			return;
		}

		m_workspaceCompiler = compiler;
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
		const QString normalized = backend.trimmed().toLower() == "slang" ? QStringLiteral("slang") : QStringLiteral("dxc");
		if (m_shaderBackend == normalized)
		{
			return;
		}
		m_shaderBackend = normalized;
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

	void LauncherSettings::SetGraphicsApi(const QString& graphicsApi)
	{
		const QString normalized = graphicsApi.trimmed().toLower() == "vulkan" ? QStringLiteral("vulkan") : QStringLiteral("d3d12");
		if (m_graphicsApi == normalized)
		{
			return;
		}
		m_graphicsApi = normalized;
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
