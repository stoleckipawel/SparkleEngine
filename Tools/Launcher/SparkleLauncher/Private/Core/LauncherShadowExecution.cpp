#include "LauncherShadowExecution.h"

#include "LauncherStatePaths.h"

#include <QtCore/QProcess>
#include <QtCore/QStringList>

#include <system_error>

#if defined(_WIN32)
  #define NOMINMAX
  #ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
  #endif
  #include <Windows.h>
#endif

namespace SparkleLauncher
{
	static std::filesystem::path CurrentLauncherExecutable()
	{
#if defined(_WIN32)
		std::wstring buffer(32768, L'\0');
		const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length == 0 || length == buffer.size())
		{
			return {};
		}
		buffer.resize(length);
		return buffer;
#else
		std::error_code errorCode;
		return std::filesystem::read_symlink("/proc/self/exe", errorCode);
#endif
	}

	static bool IsWithin(const std::filesystem::path& candidate, const std::filesystem::path& root)
	{
		const std::filesystem::path relative = candidate.lexically_relative(root);
		return !relative.empty() && *relative.begin() != "..";
	}

	static QStringList ToQStringList(const std::vector<std::string>& arguments)
	{
		QStringList result;
		for (const std::string& argument : arguments)
		{
			result.push_back(QString::fromStdString(argument));
		}
		return result;
	}

	LauncherShadowStartResult StartLauncherShadow(
	    const std::filesystem::path& repositoryRoot,
	    const std::vector<std::string>& arguments,
	    bool waitForExit)
	{
		LauncherShadowStartResult result;
		const std::filesystem::path currentExecutable = CurrentLauncherExecutable();
		if (currentExecutable.empty())
		{
			result.ErrorMessage = "Launcher executable identity could not be resolved.";
			return result;
		}

		const std::filesystem::path currentDirectory = currentExecutable.parent_path();
		const std::filesystem::path shadowRoot = ResolveLauncherStatePaths(repositoryRoot).LiveInstancesRoot;
		if (IsWithin(currentDirectory, shadowRoot))
		{
			result.State = LauncherShadowStartState::AlreadyRunningFromShadow;
			result.ExitCode = 0;
			return result;
		}

		std::error_code errorCode;
		const auto writeTime = std::filesystem::last_write_time(currentExecutable, errorCode);
		if (errorCode)
		{
			result.ErrorMessage = "Launcher executable identity failed: " + errorCode.message();
			return result;
		}

		const std::filesystem::path shadowDirectory =
		    shadowRoot / ("Generation-" + std::to_string(writeTime.time_since_epoch().count()));
		const std::filesystem::path shadowExecutable = shadowDirectory / currentExecutable.filename();
		if (!std::filesystem::exists(shadowExecutable, errorCode))
		{
			errorCode.clear();
			std::filesystem::create_directories(shadowDirectory, errorCode);
			if (!errorCode)
			{
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
				result.ErrorMessage = "Launcher shadow copy failed: " + failure;
				return result;
			}
		}

		const QString program = QString::fromStdWString(shadowExecutable.wstring());
		const QString workingDirectory = QString::fromStdWString(repositoryRoot.wstring());
		const QStringList processArguments = ToQStringList(arguments);
		if (!waitForExit)
		{
			if (!QProcess::startDetached(program, processArguments, workingDirectory))
			{
				result.ErrorMessage = "Launcher shadow restart failed: " + shadowExecutable.string();
				return result;
			}
			result.State = LauncherShadowStartState::Started;
			result.ExitCode = 0;
			return result;
		}

		QProcess process;
		process.setProgram(program);
		process.setArguments(processArguments);
		process.setWorkingDirectory(workingDirectory);
		process.setProcessChannelMode(QProcess::ForwardedChannels);
		process.start();
		if (!process.waitForStarted())
		{
			result.ErrorMessage = "Launcher shadow restart failed: " + process.errorString().toStdString();
			return result;
		}
		if (!process.waitForFinished(-1))
		{
			result.ErrorMessage = "Launcher shadow execution failed: " + process.errorString().toStdString();
			return result;
		}

		result.State = LauncherShadowStartState::Started;
		result.ExitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : 1;
		return result;
	}
}
