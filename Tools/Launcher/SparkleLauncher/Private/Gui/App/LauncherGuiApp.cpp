#include "LauncherGuiApp.h"

#include "LauncherBackend.h"
#include "LauncherMainWindow.h"
#include "LauncherContentModel.h"
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

#include <array>
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

	static std::filesystem::path RequestedRepositoryStartPath(const QStringList& arguments)
	{
		const int rootArgumentIndex = arguments.indexOf(QStringLiteral("--root"));
		if (rootArgumentIndex < 0 || rootArgumentIndex + 1 >= arguments.size())
		{
			return {};
		}

		return std::filesystem::path(arguments[rootArgumentIndex + 1].toStdString());
	}

	static std::optional<RepositoryRoot> TryResolveRepositoryRoot(
	    const std::filesystem::path& requestedStartPath,
	    std::string& outErrorMessage)
	{
		if (!requestedStartPath.empty())
		{
			return TryFindRepositoryRoot(requestedStartPath, outErrorMessage);
		}

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

		QStringList arguments = QCoreApplication::arguments();
		if (!arguments.isEmpty())
		{
			arguments.removeFirst();
		}

		if (!QProcess::startDetached(QString::fromStdString(shadowExecutable.string()), arguments))
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
		std::filesystem::path repositoryRoot = std::filesystem::current_path();
		QString startupNotice;
		const std::filesystem::path requestedStartPath = RequestedRepositoryStartPath(QCoreApplication::arguments());

		if (const std::optional<RepositoryRoot> repository = TryResolveRepositoryRoot(requestedStartPath, repositoryError))
		{
			repositoryRoot = repository->RootPath;
			std::error_code errorCode;
			std::filesystem::current_path(repositoryRoot, errorCode);
			QDir::setCurrent(QString::fromStdString(repositoryRoot.string()));

			QString shadowError;
			if (TryStartShadowLauncher(repositoryRoot, shadowError))
			{
				return 0;
			}
			if (!shadowError.isEmpty())
			{
				startupNotice = shadowError;
			}
		}
		else
		{
			startupNotice = QString::fromStdString("Repository discovery failed: " + repositoryError);
		}

		QTimer::singleShot(
		    0,
		    &application,
		    [repositoryRoot, startupNotice]()
		    {
			    auto* settings = new LauncherSettings();
			    auto* contentModel = new LauncherContentModel();
			    auto* backend = new LauncherBackend();
			    auto* mainWindow = new LauncherMainWindow(repositoryRoot, *contentModel, *settings, *backend);
			    mainWindow->SetStartupNotice(startupNotice);
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
