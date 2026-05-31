#include "LauncherGuiApp.h"

#include "LauncherBackend.h"
#include "LauncherMainWindow.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>

#include <filesystem>
#include <optional>
#include <string>

#if defined(_WIN32)
	#define NOMINMAX
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
#endif

namespace SparkleLauncher
{
	static void ForceShowWindow(LauncherMainWindow& mainWindow)
	{
		mainWindow.showNormal();
		mainWindow.raise();
		mainWindow.activateWindow();

#if defined(_WIN32)
		const HWND handle = reinterpret_cast<HWND>(mainWindow.winId());
		if (handle != nullptr)
		{
			ShowWindow(handle, SW_SHOWNORMAL);
			SetForegroundWindow(handle);
		}
#endif
	}

	static std::optional<RepositoryRoot> TryResolveRepositoryRoot(std::string& outErrorMessage)
	{
		const std::array<std::filesystem::path, 3> candidatePaths = {
		    std::filesystem::current_path(),
		    std::filesystem::path(QCoreApplication::applicationDirPath().toStdString()),
		    std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()),
		};

		std::string lastError;
		for (const std::filesystem::path& candidatePath : candidatePaths)
		{
			std::string errorMessage;
			if (const std::optional<RepositoryRoot> repository = TryFindRepositoryRoot(candidatePath, errorMessage))
			{
				outErrorMessage.clear();
				return repository;
			}

			if (!errorMessage.empty())
			{
				lastError = std::move(errorMessage);
			}
		}

		outErrorMessage = lastError;
		return std::nullopt;
	}

	int RunLauncherGui(int argc, char** argv)
	{
		QApplication application(argc, argv);
		QApplication::setApplicationName("Sparkle Launcher");
		QApplication::setOrganizationName("Sparkle Engine");

		std::string repositoryError;
		std::filesystem::path repositoryRoot = std::filesystem::current_path();
		QString startupNotice;

		if (const std::optional<RepositoryRoot> repository = TryResolveRepositoryRoot(repositoryError))
		{
			repositoryRoot = repository->RootPath;
			std::error_code errorCode;
			std::filesystem::current_path(repositoryRoot, errorCode);
			QDir::setCurrent(QString::fromStdString(repositoryRoot.string()));
		}
		else
		{
			startupNotice = QString::fromStdString("Repository discovery failed: " + repositoryError);
		}

		QTimer::singleShot(0, &application, [repositoryRoot, startupNotice]() {
			auto* settings = new LauncherSettings();
			auto* projectModel = new LauncherProjectModel();
			auto* backend = new LauncherBackend();
			auto* mainWindow = new LauncherMainWindow(repositoryRoot, *projectModel, *settings, *backend);
			mainWindow->SetStartupNotice(startupNotice);
			ForceShowWindow(*mainWindow);
			QTimer::singleShot(0, mainWindow, [mainWindow]() {
				ForceShowWindow(*mainWindow);
			});
			QTimer::singleShot(250, mainWindow, [mainWindow]() {
				ForceShowWindow(*mainWindow);
			});

			QObject::connect(mainWindow, &QObject::destroyed, settings, &QObject::deleteLater);
			QObject::connect(mainWindow, &QObject::destroyed, projectModel, &QObject::deleteLater);
			QObject::connect(mainWindow, &QObject::destroyed, backend, &QObject::deleteLater);
		});

		return QApplication::exec();
	}
}
