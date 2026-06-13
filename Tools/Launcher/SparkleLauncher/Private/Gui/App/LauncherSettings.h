#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

namespace SparkleLauncher
{
	class LauncherSettings final : public QObject
	{
		Q_OBJECT

	public:
		explicit LauncherSettings(QObject* parent = nullptr);

		const QString& BuildConfiguration() const;
		const QString& EditorProfile() const;
		const QString& RuntimeProfile() const;
		const QString& WorkspaceIde() const;
		const QString& SelectedTargets() const;
		const QString& ShaderPackages() const;
		const QString& ShaderBackend() const;
		const QString& ShaderTargetPreset() const;
		const QString& ShaderCustomTargets() const;
		const QString& ShaderCacheDirectory() const;
		const QString& ShaderDebugArtifactDirectory() const;
		const QString& LaunchBackend() const;
		const QString& LaunchTarget() const;
		const QString& LaunchStartupLevel() const;
		const QString& LaunchVSync() const;
		const QString& LaunchHighPerformanceAdapter() const;
		const QString& LaunchMeshAutoBatching() const;
		const QString& LaunchCommandLineArguments() const;
		const QString& LaunchCVars() const;
		const QString& SmokeBackend() const;
		const QString& SmokeFrameLimit() const;
		const QString& SmokeViewMode() const;
		const QString& SmokeCapturePath() const;
		const QString& FormatMode() const;
		const QString& CleanScope() const;
		bool LaunchSmokeTest() const;
		bool ShaderUseCache() const;
		bool ShaderEnableDebugInfo() const;
		bool ShaderEnableOptimizations() const;
		bool ShaderWarningsAsErrors() const;
		bool ShaderStripReflection() const;
		bool ShaderStripDebugInfo() const;
		bool ShaderWriteDebugArtifacts() const;
		bool ShaderWriteCookedShaderStats() const;
		bool ForceConfigure() const;
		bool ForceRecook() const;
		bool ConfirmForceRecook() const;
		bool ConfirmClean() const;
		bool SmokeTrace() const;
		bool SmokeSkipLevelSwitching() const;

	public slots:
		void SetBuildConfiguration(const QString& configuration);
		void SetEditorProfile(const QString& profileName);
		void SetRuntimeProfile(const QString& profileName);
		void SetWorkspaceIde(const QString& ide);
		void SetSelectedTargets(const QString& targets);
		void SetShaderPackages(const QString& packages);
		void SetShaderBackend(const QString& backend);
		void SetShaderTargetPreset(const QString& preset);
		void SetShaderCustomTargets(const QString& targets);
		void SetShaderCacheDirectory(const QString& path);
		void SetShaderDebugArtifactDirectory(const QString& path);
		void SetShaderEnableDebugInfo(bool enabled);
		void SetShaderEnableOptimizations(bool enabled);
		void SetShaderWarningsAsErrors(bool enabled);
		void SetShaderStripReflection(bool enabled);
		void SetShaderStripDebugInfo(bool enabled);
		void SetLaunchBackend(const QString& backend);
		void SetLaunchTarget(const QString& target);
		void SetLaunchStartupLevel(const QString& levelName);
		void SetLaunchVSync(const QString& value);
		void SetLaunchHighPerformanceAdapter(const QString& value);
		void SetLaunchMeshAutoBatching(const QString& value);
		void SetLaunchCommandLineArguments(const QString& arguments);
		void SetLaunchCVars(const QString& cvars);
		void SetSmokeBackend(const QString& backend);
		void SetSmokeFrameLimit(const QString& frameLimit);
		void SetSmokeViewMode(const QString& viewMode);
		void SetSmokeCapturePath(const QString& path);
		void SetFormatMode(const QString& mode);
		void SetCleanScope(const QString& scope);
		void SetLaunchSmokeTest(bool enabled);
		void SetShaderUseCache(bool enabled);
		void SetShaderWriteDebugArtifacts(bool enabled);
		void SetShaderWriteCookedShaderStats(bool enabled);
		void SetForceConfigure(bool enabled);
		void SetForceRecook(bool enabled);
		void SetConfirmForceRecook(bool enabled);
		void SetConfirmClean(bool enabled);
		void SetSmokeTrace(bool enabled);
		void SetSmokeSkipLevelSwitching(bool enabled);

	signals:
		void SettingsChanged();

	private:
		QString m_buildConfiguration = "development";
		QString m_workspaceIde = "visual-studio";
		QString m_selectedTargets;
		QString m_shaderPackages;
		QString m_shaderBackend = "auto";
		QString m_shaderTargetPreset = "default";
		QString m_shaderCustomTargets = "DxilSm66, SpirV16";
		QString m_shaderCacheDirectory;
		QString m_shaderDebugArtifactDirectory;
		QString m_launchBackend;
		QString m_launchTarget = "editor";
		QString m_launchStartupLevel = "Sponza";
		QString m_launchVSync;
		QString m_launchHighPerformanceAdapter;
		QString m_launchMeshAutoBatching;
		QString m_launchCommandLineArguments;
		QString m_launchCVars;
		QString m_smokeBackend;
		QString m_smokeFrameLimit;
		QString m_smokeViewMode;
		QString m_smokeCapturePath;
		QString m_formatMode = "check";
		QString m_cleanScope = "selected-cooked";
		bool m_launchSmokeTest = false;
		bool m_shaderUseCache = true;
		bool m_shaderEnableDebugInfo = false;
		bool m_shaderEnableOptimizations = true;
		bool m_shaderWarningsAsErrors = true;
		bool m_shaderStripReflection = true;
		bool m_shaderStripDebugInfo = true;
		bool m_shaderWriteDebugArtifacts = false;
		bool m_shaderWriteCookedShaderStats = false;
		bool m_forceConfigure = false;
		bool m_forceRecook = false;
		bool m_confirmForceRecook = false;
		bool m_confirmClean = false;
		bool m_smokeTrace = false;
		bool m_smokeSkipLevelSwitching = false;
	};
}
