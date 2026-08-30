#include "Process/ChildProcess.h"
#include "Process/ChildProcessWindows.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <utility>

#if defined(_WIN32)
  #define NOMINMAX
  #ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
  #endif
  #include <Windows.h>
#endif

class ChildProcessWindowsImplementation final
{
public:
#if defined(_WIN32)
	static constexpr DWORD ProcessTerminationTimeoutMilliseconds = 5'000;
	class Win32Handle final
	{
	public:
		Win32Handle() noexcept = default;
		explicit Win32Handle(HANDLE handle) noexcept :
		    m_handle(handle)
		{
		}
		~Win32Handle() { Reset(); }

		Win32Handle(const Win32Handle&) = delete;
		Win32Handle& operator=(const Win32Handle&) = delete;
		Win32Handle(Win32Handle&& other) noexcept :
		    m_handle(std::exchange(other.m_handle, nullptr))
		{
		}
		Win32Handle& operator=(Win32Handle&& other) noexcept
		{
			if (this != &other)
			{
				Reset();
				m_handle = std::exchange(other.m_handle, nullptr);
			}
			return *this;
		}

		HANDLE Get() const noexcept { return m_handle; }
		explicit operator bool() const noexcept { return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE; }
		void Reset(HANDLE handle = nullptr) noexcept
		{
			if (*this)
				CloseHandle(m_handle);
			m_handle = handle;
		}

	private:
		HANDLE m_handle = nullptr;
	};

	static std::wstring Utf8ToWide(std::string_view text)
	{
		if (text.empty())
			return {};
		const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
		if (length <= 0)
			return std::wstring(text.begin(), text.end());
		std::wstring result(static_cast<std::size_t>(length), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
		return result;
	}

	static std::wstring QuoteArgument(const std::wstring& argument)
	{
		if (argument.empty())
			return L"\"\"";
		const bool needsQuotes =
		    std::any_of(argument.begin(), argument.end(), [](wchar_t value) { return std::iswspace(value) || value == L'\"'; });
		if (!needsQuotes)
			return argument;

		std::wstring quoted(1, L'\"');
		std::size_t backslashes = 0;
		for (const wchar_t value : argument)
		{
			if (value == L'\\')
			{
				++backslashes;
				continue;
			}
			if (value == L'\"')
			{
				quoted.append(backslashes * 2 + 1, L'\\');
				quoted.push_back(value);
				backslashes = 0;
				continue;
			}
			quoted.append(backslashes, L'\\');
			backslashes = 0;
			quoted.push_back(value);
		}
		quoted.append(backslashes * 2, L'\\');
		quoted.push_back(L'\"');
		return quoted;
	}

	static std::wstring BuildCommandLine(const Process::ChildProcessRequest& request)
	{
		std::wstring commandLine = QuoteArgument(request.ExecutablePath.wstring());
		for (const std::string& argument : request.Arguments)
		{
			commandLine.push_back(L' ');
			commandLine += QuoteArgument(Utf8ToWide(argument));
		}
		return commandLine;
	}

	static std::vector<wchar_t> BuildEnvironment(const std::vector<Process::EnvironmentOverride>& overrides)
	{
		if (overrides.empty())
			return {};
		std::vector<std::wstring> entries;
		if (LPWCH environment = GetEnvironmentStringsW())
		{
			for (const wchar_t* current = environment; *current != L'\0'; current += std::wcslen(current) + 1)
				entries.emplace_back(current);
			FreeEnvironmentStringsW(environment);
		}
		for (const Process::EnvironmentOverride& overrideValue : overrides)
		{
			const std::wstring prefix = Utf8ToWide(overrideValue.Name) + L"=";
			const std::wstring replacement = prefix + Utf8ToWide(overrideValue.Value);
			auto entry =
			    std::find_if(entries.begin(), entries.end(), [&prefix](const std::wstring& value) { return value.rfind(prefix, 0) == 0; });
			if (entry == entries.end())
				entries.push_back(replacement);
			else
				*entry = replacement;
		}
		std::sort(entries.begin(), entries.end());
		std::vector<wchar_t> block;
		for (const std::wstring& entry : entries)
		{
			block.insert(block.end(), entry.begin(), entry.end());
			block.push_back(L'\0');
		}
		block.push_back(L'\0');
		return block;
	}

	static std::string FormatError(DWORD code)
	{
		LPSTR raw = nullptr;
		const DWORD length = FormatMessageA(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		    nullptr,
		    code,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    reinterpret_cast<LPSTR>(&raw),
		    0,
		    nullptr);
		std::string message = length != 0 && raw != nullptr ? std::string(raw, length) : "Unknown Windows error";
		if (raw != nullptr)
			LocalFree(raw);
		return message;
	}

	static void ConsumeOutput(
	    Process::ChildProcessResult& result,
	    const Process::ChildProcessRequest& request,
	    std::ofstream& log,
	    const char* data,
	    std::size_t size)
	{
		result.CapturedOutput.append(data, size);
		if (request.OutputCallback)
			request.OutputCallback(std::string_view(data, size));
		if (log.is_open())
			log.write(data, static_cast<std::streamsize>(size));
	}
#endif
};

Process::ChildProcessResult Process::Detail::RunWindowsChildProcess(const ChildProcessRequest& request)
{
	ChildProcessResult result;
#if defined(_WIN32)
	if (request.ExecutablePath.empty())
	{
		result.FailureReason = "Child process executable path is empty.";
		return result;
	}
	if (request.Cancellation.stop_requested())
	{
		result.Cancelled = true;
		return result;
	}

	static std::atomic_uint64_t nextPipeIdentity{1};
	const std::wstring pipeName = L"\\\\.\\pipe\\Sparkle.ChildProcess." + std::to_wstring(GetCurrentProcessId()) + L"."
	    + std::to_wstring(nextPipeIdentity.fetch_add(1, std::memory_order_relaxed));
	ChildProcessWindowsImplementation::Win32Handle readPipe(CreateNamedPipeW(
	    pipeName.c_str(),
	    PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
	    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
	    1,
	    64 * 1024,
	    64 * 1024,
	    0,
	    nullptr));
	if (!readPipe)
	{
		result.FailureReason = "Failed to create child output pipe: " + ChildProcessWindowsImplementation::FormatError(GetLastError());
		return result;
	}

	SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
	ChildProcessWindowsImplementation::Win32Handle writePipe(
	    CreateFileW(pipeName.c_str(), GENERIC_WRITE, 0, &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	if (!writePipe)
	{
		result.FailureReason = "Failed to open child output pipe: " + ChildProcessWindowsImplementation::FormatError(GetLastError());
		return result;
	}
	if (!ConnectNamedPipe(readPipe.Get(), nullptr) && GetLastError() != ERROR_PIPE_CONNECTED)
	{
		result.FailureReason = "Failed to connect child output pipe: " + ChildProcessWindowsImplementation::FormatError(GetLastError());
		return result;
	}

	std::ofstream log;
	if (!request.LogPath.empty())
	{
		std::error_code error;
		std::filesystem::create_directories(request.LogPath.parent_path(), error);
		log.open(request.LogPath, std::ios::binary | std::ios::trunc);
	}

	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;
	startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	startup.hStdOutput = writePipe.Get();
	startup.hStdError = writePipe.Get();
	PROCESS_INFORMATION information{};
	std::wstring commandLine = ChildProcessWindowsImplementation::BuildCommandLine(request);
	std::wstring workingDirectory = request.WorkingDirectory.wstring();
	std::vector<wchar_t> environment = ChildProcessWindowsImplementation::BuildEnvironment(request.Environment);
	if (!CreateProcessW(
	        nullptr,
	        commandLine.data(),
	        nullptr,
	        nullptr,
	        TRUE,
	        CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED,
	        environment.empty() ? nullptr : environment.data(),
	        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
	        &startup,
	        &information))
	{
		result.FailureReason = "Failed to launch child process: " + ChildProcessWindowsImplementation::FormatError(GetLastError());
		return result;
	}
	result.Launched = true;
	ChildProcessWindowsImplementation::Win32Handle process(information.hProcess);
	ChildProcessWindowsImplementation::Win32Handle processThread(information.hThread);
	ChildProcessWindowsImplementation::Win32Handle processJob(CreateJobObjectW(nullptr, nullptr));
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobLimits{};
	jobLimits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!processJob
	    || !SetInformationJobObject(processJob.Get(), JobObjectExtendedLimitInformation, &jobLimits, static_cast<DWORD>(sizeof(jobLimits)))
	    || !AssignProcessToJobObject(processJob.Get(), process.Get()))
	{
		const DWORD ownershipError = GetLastError();
		TerminateProcess(process.Get(), 1);
		WaitForSingleObject(process.Get(), ChildProcessWindowsImplementation::ProcessTerminationTimeoutMilliseconds);
		result.FailureReason =
		    "Failed to establish child process tree ownership: " + ChildProcessWindowsImplementation::FormatError(ownershipError);
		return result;
	}
	if (ResumeThread(processThread.Get()) == static_cast<DWORD>(-1))
	{
		const DWORD resumeError = GetLastError();
		TerminateJobObject(processJob.Get(), 1);
		WaitForSingleObject(process.Get(), ChildProcessWindowsImplementation::ProcessTerminationTimeoutMilliseconds);
		result.FailureReason = "Failed to start child process: " + ChildProcessWindowsImplementation::FormatError(resumeError);
		return result;
	}
	writePipe.Reset();

	ChildProcessWindowsImplementation::Win32Handle readEvent(CreateEventW(nullptr, TRUE, FALSE, nullptr));
	ChildProcessWindowsImplementation::Win32Handle cancelEvent(CreateEventW(nullptr, TRUE, request.Cancellation.stop_requested(), nullptr));
	if (!readEvent || !cancelEvent)
	{
		const DWORD eventError = GetLastError();
		TerminateJobObject(processJob.Get(), 1);
		WaitForSingleObject(process.Get(), ChildProcessWindowsImplementation::ProcessTerminationTimeoutMilliseconds);
		result.FailureReason =
		    "Failed to create child process completion events: " + ChildProcessWindowsImplementation::FormatError(eventError);
		return result;
	}
	std::stop_callback cancellationWake(request.Cancellation, [event = cancelEvent.Get()] { SetEvent(event); });

	std::array<char, 4096> buffer{};
	OVERLAPPED readOperation{};
	readOperation.hEvent = readEvent.Get();
	bool readPending = false;
	bool pipeClosed = false;
	bool processExited = false;
	bool terminationRequested = false;
	while (!processExited || !pipeClosed)
	{
		if (!pipeClosed && !readPending)
		{
			ResetEvent(readEvent.Get());
			DWORD bytesRead = 0;
			if (ReadFile(readPipe.Get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, &readOperation))
			{
				if (bytesRead == 0)
					pipeClosed = true;
				else
					ChildProcessWindowsImplementation::ConsumeOutput(result, request, log, buffer.data(), bytesRead);
				continue;
			}
			const DWORD readError = GetLastError();
			if (readError == ERROR_IO_PENDING)
				readPending = true;
			else if (readError == ERROR_BROKEN_PIPE)
				pipeClosed = true;
			else
			{
				result.FailureReason = "Failed while reading child output: " + ChildProcessWindowsImplementation::FormatError(readError);
				pipeClosed = true;
			}
		}

		std::array<HANDLE, 3> waits{};
		DWORD processIndex = MAXDWORD;
		DWORD readIndex = MAXDWORD;
		DWORD cancelIndex = MAXDWORD;
		DWORD count = 0;
		if (!processExited)
		{
			processIndex = count;
			waits[count++] = process.Get();
		}
		if (readPending)
		{
			readIndex = count;
			waits[count++] = readEvent.Get();
		}
		if (!terminationRequested)
		{
			cancelIndex = count;
			waits[count++] = cancelEvent.Get();
		}
		if (count == 0)
			break;
		const DWORD waitResult = WaitForMultipleObjects(count, waits.data(), FALSE, INFINITE);
		if (waitResult == WAIT_FAILED)
		{
			result.FailureReason =
			    "Failed while waiting for child process completion: " + ChildProcessWindowsImplementation::FormatError(GetLastError());
			TerminateJobObject(processJob.Get(), 1);
			WaitForSingleObject(process.Get(), ChildProcessWindowsImplementation::ProcessTerminationTimeoutMilliseconds);
			break;
		}
		const DWORD signalled = waitResult - WAIT_OBJECT_0;
		if (signalled == processIndex)
		{
			processExited = true;
			continue;
		}
		if (signalled == cancelIndex)
		{
			result.Cancelled = true;
			terminationRequested = true;
			TerminateJobObject(processJob.Get(), 1);
			continue;
		}
		if (signalled == readIndex)
		{
			DWORD bytesRead = 0;
			readPending = false;
			if (GetOverlappedResult(readPipe.Get(), &readOperation, &bytesRead, FALSE) && bytesRead != 0)
				ChildProcessWindowsImplementation::ConsumeOutput(result, request, log, buffer.data(), bytesRead);
			else if (GetLastError() == ERROR_BROKEN_PIPE || bytesRead == 0)
				pipeClosed = true;
			else
			{
				result.FailureReason =
				    "Failed while completing child output read: " + ChildProcessWindowsImplementation::FormatError(GetLastError());
				pipeClosed = true;
			}
		}
	}

	DWORD exitCode = 1;
	GetExitCodeProcess(process.Get(), &exitCode);
	result.ExitCode = static_cast<int>(exitCode);
#endif
	return result;
}
