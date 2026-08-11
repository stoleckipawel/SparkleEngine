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

		const QString& RunMode() const;
		const QString& BuildConfiguration() const;
		const QString& EditorProfile() const;
		const QString& RuntimeProfile() const;
		const QString& WorkspaceIde() const;
		const QString& WorkspaceCompiler() const;
		const QString& BuildScopes() const;
		const QString& CookScopes() const;
		const QString& SelectedTargets() const;
		const QString& ShaderBackend() const;
		const QString& ShaderCacheDirectory() const;
		const QString& GraphicsApi() const;
		const QString& CleanScope() const;
		bool ShaderUseCache() const;
		bool ShaderEnableDebugInfo() const;
		bool ShaderEnableOptimizations() const;
		bool ShaderWarningsAsErrors() const;
		bool ShaderStripDebugInfo() const;
		bool ForceConfigure() const;
		bool ForceRecook() const;
		bool ConfirmForceRecook() const;
		bool ConfirmClean() const;

	public slots:
		void SetRunMode(const QString& runMode);
		void SetBuildConfiguration(const QString& configuration);
		void SetEditorProfile(const QString& profileName);
		void SetRuntimeProfile(const QString& profileName);
		void SetWorkspaceIde(const QString& ide);
		void SetWorkspaceCompiler(const QString& compiler);
		void SetBuildScopes(const QString& scopes);
		void SetCookScopes(const QString& scopes);
		void SetSelectedTargets(const QString& targets);
		void SetShaderBackend(const QString& backend);
		void SetShaderCacheDirectory(const QString& path);
		void SetGraphicsApi(const QString& graphicsApi);
		void SetShaderEnableDebugInfo(bool enabled);
		void SetShaderEnableOptimizations(bool enabled);
		void SetShaderWarningsAsErrors(bool enabled);
		void SetShaderStripDebugInfo(bool enabled);
		void SetCleanScope(const QString& scope);
		void SetShaderUseCache(bool enabled);
		void SetForceConfigure(bool enabled);
		void SetForceRecook(bool enabled);
		void SetConfirmForceRecook(bool enabled);
		void SetConfirmClean(bool enabled);

	signals:
		void SettingsChanged();

	private:
		QString m_runMode = "editor";
		QString m_buildConfiguration = "development";
		QString m_workspaceIde = "rider";
		QString m_workspaceCompiler = "msvc";
		QString m_buildScopes = "editor;runtime;cook-tools";
		QString m_cookScopes = "shaders;textures;assets";
		QString m_selectedTargets;
		QString m_shaderBackend = "dxc";
		QString m_shaderCacheDirectory;
		QString m_graphicsApi = "d3d12";
		QString m_cleanScope = "cooked";
		bool m_shaderUseCache = true;
		bool m_shaderEnableDebugInfo = false;
		bool m_shaderEnableOptimizations = true;
		bool m_shaderWarningsAsErrors = true;
		bool m_shaderStripDebugInfo = true;
		bool m_forceConfigure = false;
		bool m_forceRecook = false;
		bool m_confirmForceRecook = false;
		bool m_confirmClean = false;
	};
}
