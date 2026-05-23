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
}