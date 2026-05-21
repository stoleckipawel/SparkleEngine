#include "SparkleLauncher/ProcessRunner.h"

#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <sstream>
#include <system_error>
#include <thread>
#include <utility>

#if defined(_WIN32)
	#define NOMINMAX
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
#endif

namespace SparkleLauncher
{
	static std::string QuoteDisplayArgument(std::string_view argument)
	{
		if (argument.empty())
		{
			return "\"\"";
		}

		bool needsQuotes = false;
		for (const char character : argument)
		{
			if (character == ' ' || character == '\t' || character == '"')
			{
				needsQuotes = true;
				break;
			}
		}

		if (!needsQuotes)
		{
			return std::string(argument);
		}

		std::string quoted;
		quoted.push_back('"');
		for (const char character : argument)
		{
			if (character == '"')
			{
				quoted.push_back('\\');
			}
			quoted.push_back(character);
		}
		quoted.push_back('"');
		return quoted;
	}

#if defined(_WIN32)
	static std::wstring Utf8ToWide(std::string_view text)
	{
		if (text.empty())
		{
			return {};
		}

		const int requiredLength = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
		if (requiredLength <= 0)
		{
			return std::wstring(text.begin(), text.end());
		}

		std::wstring result(static_cast<std::size_t>(requiredLength), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), requiredLength);
		return result;
	}

	static std::wstring QuoteWindowsArgument(const std::wstring& argument)
	{
		if (argument.empty())
		{
			return L"\"\"";
		}

		bool needsQuotes = false;
		for (const wchar_t character : argument)
		{
			if (iswspace(character) || character == L'"')
			{
				needsQuotes = true;
				break;
			}
		}

		if (!needsQuotes)
		{
			return argument;
		}

		std::wstring quoted;
		quoted.push_back(L'"');
		int backslashCount = 0;
		for (const wchar_t character : argument)
		{
			if (character == L'\\')
			{
				++backslashCount;
				continue;
			}

			if (character == L'"')
			{
				quoted.append(static_cast<std::size_t>(backslashCount * 2 + 1), L'\\');
				quoted.push_back(character);
				backslashCount = 0;
				continue;
			}

			quoted.append(static_cast<std::size_t>(backslashCount), L'\\');
			backslashCount = 0;
			quoted.push_back(character);
		}

		quoted.append(static_cast<std::size_t>(backslashCount * 2), L'\\');
		quoted.push_back(L'"');
		return quoted;
	}

	static std::wstring BuildWindowsCommandLine(const ProcessRequest& request)
	{
		std::wstring commandLine = QuoteWindowsArgument(request.ExecutablePath.wstring());
		for (const std::string& argument : request.Arguments)
		{
			commandLine.push_back(L' ');
			commandLine += QuoteWindowsArgument(Utf8ToWide(argument));
		}
		return commandLine;
	}

	static std::string FormatWindowsError(DWORD errorCode)
	{
		LPSTR rawMessage = nullptr;
		const DWORD length = FormatMessageA(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		    nullptr,
		    errorCode,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    reinterpret_cast<LPSTR>(&rawMessage),
		    0,
		    nullptr);

		std::string message = length > 0 && rawMessage != nullptr ? std::string(rawMessage, length) : "Unknown Windows error";
		if (rawMessage != nullptr)
		{
			LocalFree(rawMessage);
		}
		return message;
	}

	static std::vector<wchar_t> BuildEnvironmentBlock(const std::vector<EnvironmentOverride>& overrides)
	{
		if (overrides.empty())
		{
			return {};
		}

		std::vector<std::wstring> entries;
		LPWCH environmentStrings = GetEnvironmentStringsW();
		if (environmentStrings != nullptr)
		{
			for (const wchar_t* current = environmentStrings; *current != L'\0'; current += wcslen(current) + 1)
			{
				entries.emplace_back(current);
			}
			FreeEnvironmentStringsW(environmentStrings);
		}

		for (const EnvironmentOverride& overrideValue : overrides)
		{
			const std::wstring name = Utf8ToWide(overrideValue.Name);
			const std::wstring prefix = name + L"=";
			const std::wstring replacement = prefix + Utf8ToWide(overrideValue.Value);
			auto found = std::find_if(entries.begin(), entries.end(), [&prefix](const std::wstring& entry) {
				return entry.rfind(prefix, 0) == 0;
			});

			if (found == entries.end())
			{
				entries.push_back(replacement);
			}
			else
			{
				*found = replacement;
			}
		}

		std::vector<wchar_t> block;
		for (const std::wstring& entry : entries)
		{
			block.insert(block.end(), entry.begin(), entry.end());
			block.push_back(L'\0');
		}
		block.push_back(L'\0');
		return block;
	}
#endif

	void CancellationToken::RequestCancel() noexcept
	{
		m_cancelRequested.store(true);
	}

	bool CancellationToken::IsCancellationRequested() const noexcept
	{
		return m_cancelRequested.load();
	}

	ProcessResult NativeProcessRunner::Run(const ProcessRequest& request)
	{
		ProcessResult result;
		result.StartTime = std::chrono::system_clock::now();

#if defined(_WIN32)
		SECURITY_ATTRIBUTES securityAttributes = {};
		securityAttributes.nLength = sizeof(securityAttributes);
		securityAttributes.bInheritHandle = TRUE;

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0))
		{
			result.FailureReason = "Failed to create process output pipe: " + FormatWindowsError(GetLastError());
			result.EndTime = std::chrono::system_clock::now();
			return result;
		}

		if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0))
		{
			result.FailureReason = "Failed to configure process output pipe: " + FormatWindowsError(GetLastError());
			CloseHandle(writePipe);
			CloseHandle(readPipe);
			result.EndTime = std::chrono::system_clock::now();
			return result;
		}

		std::ofstream logStream;
		if (!request.LogPath.empty())
		{
			std::error_code directoryError;
			std::filesystem::create_directories(request.LogPath.parent_path(), directoryError);
			logStream.open(request.LogPath, std::ios::binary | std::ios::out | std::ios::trunc);
		}

		STARTUPINFOW startupInfo = {};
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags = STARTF_USESTDHANDLES;
		startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		startupInfo.hStdOutput = writePipe;
		startupInfo.hStdError = writePipe;

		PROCESS_INFORMATION processInformation = {};
		std::wstring commandLine = BuildWindowsCommandLine(request);
		std::wstring workingDirectory = request.WorkingDirectory.empty() ? std::wstring() : request.WorkingDirectory.wstring();
		std::vector<wchar_t> environmentBlock = BuildEnvironmentBlock(request.Environment);

		const BOOL createdProcess = CreateProcessW(
		    nullptr,
		    commandLine.data(),
		    nullptr,
		    nullptr,
		    TRUE,
		    CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
		    environmentBlock.empty() ? nullptr : environmentBlock.data(),
		    workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
		    &startupInfo,
		    &processInformation);

		CloseHandle(writePipe);

		if (!createdProcess)
		{
			result.FailureReason = "Failed to launch process: " + FormatWindowsError(GetLastError());
			CloseHandle(readPipe);
			result.EndTime = std::chrono::system_clock::now();
			return result;
		}

		result.Launched = true;
		std::thread readerThread([&result, &request, &logStream, readPipe]() {
			char buffer[4096];
			DWORD bytesRead = 0;
			while (ReadFile(readPipe, buffer, static_cast<DWORD>(sizeof(buffer)), &bytesRead, nullptr) && bytesRead > 0)
			{
				std::string_view chunk(buffer, static_cast<std::size_t>(bytesRead));
				result.CapturedOutput.append(chunk.data(), chunk.size());
				if (request.OutputCallback)
				{
					request.OutputCallback(chunk);
				}
				if (logStream.is_open())
				{
					logStream.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
				}
			}
		});

		while (true)
		{
			const DWORD waitResult = WaitForSingleObject(processInformation.hProcess, 50);
			if (waitResult == WAIT_OBJECT_0)
			{
				break;
			}

			if (request.Cancellation != nullptr && request.Cancellation->IsCancellationRequested())
			{
				result.Canceled = true;
				TerminateProcess(processInformation.hProcess, 1);
				WaitForSingleObject(processInformation.hProcess, INFINITE);
				break;
			}
		}

		DWORD exitCode = 1;
		GetExitCodeProcess(processInformation.hProcess, &exitCode);
		result.ExitCode = static_cast<int>(exitCode);

		CloseHandle(processInformation.hThread);
		CloseHandle(processInformation.hProcess);

		if (readerThread.joinable())
		{
			readerThread.join();
		}

		CloseHandle(readPipe);
#else
		result.FailureReason = "Native process execution is not implemented for this platform yet.";
#endif

		result.EndTime = std::chrono::system_clock::now();
		return result;
	}

	std::string BuildDisplayCommandLine(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments)
	{
		std::ostringstream commandLine;
		commandLine << QuoteDisplayArgument(executablePath.string());
		for (const std::string& argument : arguments)
		{
			commandLine << ' ' << QuoteDisplayArgument(argument);
		}
		return commandLine.str();
	}
}