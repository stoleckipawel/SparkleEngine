#pragma once

#include "LauncherBackend.h"

#include <QtCore/QString>
#include <filesystem>

class QWidget;

namespace SparkleLauncher
{
	class LauncherProjectModel;
	class LauncherSettings;

	struct LauncherPrerequisiteDecision
	{
		enum class Kind
		{
			Ready,
			Blocked,
			RunPrerequisite
		};

		Kind Result = Kind::Ready;
		LauncherOperationRequest Request;
		QString Title;
		QString StatusMessage;
	};

	LauncherPrerequisiteDecision ResolveWorkspacePrerequisitePrompt(
	    QWidget* parent,
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const QString& operationId);

	LauncherPrerequisiteDecision ResolveLaunchPrerequisitePrompt(
	    QWidget* parent,
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const QString& operationId);

	LauncherPrerequisiteDecision ResolveCookPrerequisitePrompt(
	    QWidget* parent,
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const QString& operationId);
}
