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

		const QString& EditorProfile() const;
		const QString& RuntimeProfile() const;
		const QString& SelectedTargets() const;
		const QString& ShaderPackages() const;
		const QString& LaunchBackend() const;
		const QString& LaunchVSync() const;
		const QString& LaunchHighPerformanceAdapter() const;
		const QString& LaunchMeshAutoBatching() const;
		const QString& LaunchCommandLineArguments() const;
		const QString& LaunchCVars() const;
		const QString& SmokeBackend() const;
		const QString& SmokeFrameLimit() const;
		const QString& FormatMode() const;
		const QString& CleanScope() const;
		bool ForceConfigure() const;
		bool ForceRecook() const;
		bool ConfirmForceRecook() const;
		bool ConfirmClean() const;
		bool SmokeTrace() const;
		bool SmokeSkipLevelSwitching() const;

	public slots:
		void SetEditorProfile(const QString& profileName);
		void SetRuntimeProfile(const QString& profileName);
		void SetSelectedTargets(const QString& targets);
		void SetShaderPackages(const QString& packages);
		void SetLaunchBackend(const QString& backend);
		void SetLaunchVSync(const QString& value);
		void SetLaunchHighPerformanceAdapter(const QString& value);
		void SetLaunchMeshAutoBatching(const QString& value);
		void SetLaunchCommandLineArguments(const QString& arguments);
		void SetLaunchCVars(const QString& cvars);
		void SetSmokeBackend(const QString& backend);
		void SetSmokeFrameLimit(const QString& frameLimit);
		void SetFormatMode(const QString& mode);
		void SetCleanScope(const QString& scope);
		void SetForceConfigure(bool enabled);
		void SetForceRecook(bool enabled);
		void SetConfirmForceRecook(bool enabled);
		void SetConfirmClean(bool enabled);
		void SetSmokeTrace(bool enabled);
		void SetSmokeSkipLevelSwitching(bool enabled);

	signals:
		void SettingsChanged();

	private:
		QString m_editorProfile = "DevelopmentEditor";
		QString m_runtimeProfile = "DevelopmentGame";
		QString m_selectedTargets;
		QString m_shaderPackages;
		QString m_launchBackend;
		QString m_launchVSync;
		QString m_launchHighPerformanceAdapter;
		QString m_launchMeshAutoBatching;
		QString m_launchCommandLineArguments;
		QString m_launchCVars;
		QString m_smokeBackend;
		QString m_smokeFrameLimit;
		QString m_formatMode = "check";
		QString m_cleanScope = "selected-cooked";
		bool m_forceConfigure = false;
		bool m_forceRecook = false;
		bool m_confirmForceRecook = false;
		bool m_confirmClean = false;
		bool m_smokeTrace = false;
		bool m_smokeSkipLevelSwitching = false;
	};
}