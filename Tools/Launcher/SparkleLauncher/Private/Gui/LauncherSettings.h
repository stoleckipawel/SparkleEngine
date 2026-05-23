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

	public slots:
		void SetEditorProfile(const QString& profileName);
		void SetRuntimeProfile(const QString& profileName);

	signals:
		void SettingsChanged();

	private:
		QString m_editorProfile = "DevelopmentEditor";
		QString m_runtimeProfile = "DevelopmentGame";
	};
}