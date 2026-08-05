#include "LauncherGuiApp.h"

#include "LauncherBackend.h"
#include "LauncherMainWindow.h"
#include "LauncherContentModel.h"
#include "LauncherRepositoryContext.h"
#include "LauncherSettings.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

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

	static bool TryStartShadowLauncher(const std::filesystem::path& repositoryRoot, QString& outError)
	{
		const std::filesystem::path currentDirectory = std::filesystem::path(QCoreApplication::applicationDirPath().toStdString());
		const std::filesystem::path shadowRoot = GetLauncherStateDirectory(repositoryRoot) / "Live";
		const std::filesystem::path relativeToShadow = currentDirectory.lexically_relative(shadowRoot);
		if (!relativeToShadow.empty() && *relativeToShadow.begin() != "..")
		{
			return false;
		}

		const std::filesystem::path currentExecutable = std::filesystem::path(QCoreApplication::applicationFilePath().toStdString());
		std::error_code errorCode;
		const auto writeTime = std::filesystem::last_write_time(currentExecutable, errorCode);
		if (errorCode)
		{
			outError = QStringLiteral("Launcher executable identity failed: %1").arg(QString::fromStdString(errorCode.message()));
			return false;
		}
		const std::filesystem::path shadowDirectory = shadowRoot / ("Generation-" + std::to_string(writeTime.time_since_epoch().count()));
		const std::filesystem::path shadowExecutable = shadowDirectory / currentExecutable.filename();

		errorCode.clear();
		if (!std::filesystem::exists(shadowExecutable, errorCode))
		{
			errorCode.clear();
			std::filesystem::create_directories(shadowDirectory, errorCode);
			if (!errorCode)
			{
				errorCode.clear();
				std::filesystem::copy(
				    currentDirectory,
				    shadowDirectory,
				    std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing,
				    errorCode);
			}
			if (errorCode)
			{
				const std::string failure = errorCode.message();
				errorCode.clear();
				std::filesystem::remove_all(shadowDirectory, errorCode);
				outError = QStringLiteral("Launcher shadow copy failed: %1").arg(QString::fromStdString(failure));
				return false;
			}
		}
		if (!QProcess::startDetached(
		        QString::fromStdString(shadowExecutable.string()),
		        {},
		        QString::fromStdString(repositoryRoot.string())))
		{
			outError = QStringLiteral("Launcher shadow restart failed: %1").arg(QString::fromStdString(shadowExecutable.string()));
			return false;
		}

		return true;
	}

	int RunLauncherGui(int argc, char** argv)
	{
		QApplication application(argc, argv);
		QApplication::setApplicationName("Sparkle Launcher");
		QApplication::setOrganizationName("Sparkle Engine");

		std::string repositoryError;
		const std::optional<RepositoryRoot> repository =
		    TryReadLauncherRepositoryContext(std::filesystem::path(QCoreApplication::applicationDirPath().toStdString()), repositoryError);
		if (!repository)
		{
			QMessageBox::critical(
			    nullptr,
			    QStringLiteral("Sparkle Launcher"),
			    QString::fromStdString("Launcher startup failed. " + repositoryError));
			return 1;
		}

		const std::filesystem::path repositoryRoot = repository->RootPath;
		std::error_code errorCode;
		std::filesystem::current_path(repositoryRoot, errorCode);
		if (errorCode || !QDir::setCurrent(QString::fromStdString(repositoryRoot.string())))
		{
			QMessageBox::critical(
			    nullptr,
			    QStringLiteral("Sparkle Launcher"),
			    QString::fromStdString(
			        "Launcher startup failed. Repository working directory could not be selected: " + repositoryRoot.string()));
			return 1;
		}

		QString shadowError;
		if (TryStartShadowLauncher(repositoryRoot, shadowError))
		{
			return 0;
		}
		if (!shadowError.isEmpty())
		{
			QMessageBox::critical(nullptr, QStringLiteral("Sparkle Launcher"), shadowError);
			return 1;
		}

		QTimer::singleShot(
		    0,
		    &application,
		    [repositoryRoot]()
		    {
			    auto* settings = new LauncherSettings();
			    auto* contentModel = new LauncherContentModel();
			    auto* backend = new LauncherBackend();
			    auto* mainWindow = new LauncherMainWindow(repositoryRoot, *contentModel, *settings, *backend);
			    ForceShowWindow(*mainWindow);
			    QTimer::singleShot(0, mainWindow, [mainWindow]() { ForceShowWindow(*mainWindow); });
			    QTimer::singleShot(250, mainWindow, [mainWindow]() { ForceShowWindow(*mainWindow); });

			    QObject::connect(mainWindow, &QObject::destroyed, settings, &QObject::deleteLater);
			    QObject::connect(mainWindow, &QObject::destroyed, contentModel, &QObject::deleteLater);
			    QObject::connect(mainWindow, &QObject::destroyed, backend, &QObject::deleteLater);
		    });

		return QApplication::exec();
	}
}
