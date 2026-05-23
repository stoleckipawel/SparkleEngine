#include "LauncherGuiApp.h"

#include "LauncherBackend.h"
#include "LauncherMainWindow.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <QtWidgets/QApplication>

#include <array>
#include <filesystem>
#include <optional>
#include <string>

namespace SparkleLauncher
{
	int RunLauncherGui()
	{
		int argumentCount = 1;
		std::array<char*, 1> arguments = {const_cast<char*>("SparkleLauncher")};
		QApplication application(argumentCount, arguments.data());
		QApplication::setApplicationName("Sparkle Launcher");
		QApplication::setOrganizationName("Sparkle Engine");

		std::string repositoryError;
		const std::filesystem::path startPath = std::filesystem::current_path();
		std::filesystem::path repositoryRoot = startPath;
		QString startupNotice;

		if (const std::optional<RepositoryRoot> repository = TryFindRepositoryRoot(startPath, repositoryError))
		{
			repositoryRoot = repository->RootPath;
		}
		else
		{
			startupNotice = QString::fromStdString("Repository discovery failed: " + repositoryError);
		}

		LauncherSettings settings;
		LauncherProjectModel projectModel;
		LauncherBackend backend;
		LauncherMainWindow mainWindow(repositoryRoot, projectModel, settings, backend);
		mainWindow.SetStartupNotice(startupNotice);
		mainWindow.show();

		return QApplication::exec();
	}
}