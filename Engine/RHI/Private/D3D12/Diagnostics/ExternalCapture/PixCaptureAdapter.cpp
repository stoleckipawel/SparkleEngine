#include "PCH.h"
#include "Diagnostics/ExternalCapture/ExternalCaptureAdapter.h"

#include <Windows.h>
#include <array>
#include <chrono>

#if SPARKLE_RHI_WITH_PIX_CAPTURE
  #include <pix3.h>

class PixCaptureAdapter final : public ExternalCaptureAdapter
{
public:
	explicit PixCaptureAdapter(std::filesystem::path tool) :
	    m_tool(std::move(tool))
	{
	}

	~PixCaptureAdapter() noexcept override
	{
		if (m_inspector)
		{
			CloseHandle(m_inspector);
		}
	}

	bool Begin(const ExternalCaptureNativeTarget& target, const std::filesystem::path& path, std::string& error) override
	{
		m_artifact = path;
		m_artifact += L".wpix";

		if (FAILED(PIXSetTargetWindow(static_cast<HWND>(target.Window))) || FAILED(PIXGpuCaptureNextFrames(m_artifact.c_str(), 1)))
		{
			error = "PIX rejected the native target or capture request.";
			return false;
		}
		m_retryAt = std::chrono::steady_clock::now();
		return true;
	}

	void End(const ExternalCaptureNativeTarget&) noexcept override {}

	ExternalCaptureNativeResult Poll() override
	{
		if (m_inspector != nullptr)
		{
			return PollNativeInspector();
		}
		if (std::chrono::steady_clock::now() < m_retryAt)
		{
			return {};
		}

		TryStartNativeInspector();
		return {};
	}

private:
	ExternalCaptureNativeResult PollNativeInspector()
	{
		DWORD exitCode = STILL_ACTIVE;
		if (!GetExitCodeProcess(m_inspector, &exitCode) || exitCode == STILL_ACTIVE)
		{
			return {};
		}

		CloseHandle(m_inspector);
		m_inspector = nullptr;
		if (exitCode != 0)
		{
			m_retryAt = std::chrono::steady_clock::now() + std::chrono::seconds(1);
			return {};
		}

		const bool opened = reinterpret_cast<std::intptr_t>(PIXOpenCaptureInUI(m_artifact.c_str())) > 32;

		return {
		    .State = ExternalCaptureState::Completed,
		    .Artifact = m_artifact,
		    .Message = opened ? "PIX native open confirmed finalization; capture handed to PIX." : "PIX native open confirmed finalization; viewer handoff failed. Open the artifact manually."};
	}

	void TryStartNativeInspector()
	{
		m_retryAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
		// An unlocked file permits a native-open attempt; it does not prove completion.
		HANDLE file = CreateFileW(m_artifact.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			return;
		}

		CloseHandle(file);
		std::wstring command = L"\"" + m_tool.wstring() + L"\" open-capture \"" + m_artifact.wstring() + L"\"";
		STARTUPINFOW startup{};
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESHOWWINDOW;
		startup.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION process{};
		if (CreateProcessW(m_tool.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, m_tool.parent_path().c_str(), &startup, &process))
		{
			CloseHandle(process.hThread);
			m_inspector = process.hProcess;
		}
		else
		{
			m_retryAt = std::chrono::steady_clock::now() + std::chrono::seconds(1);
		}
	}

	std::filesystem::path m_tool;
	std::filesystem::path m_artifact;
	HANDLE m_inspector = nullptr;
	std::chrono::steady_clock::time_point m_retryAt;
};
#endif

std::unique_ptr<ExternalCaptureAdapter> CreatePixCaptureAdapter(std::string& error)
{
#if SPARKLE_RHI_WITH_PIX_CAPTURE
	const HMODULE module = PIXLoadLatestWinPixGpuCapturerLibrary();
	if (!module)
	{
		error = "PIX GPU Capturer is not installed or could not be loaded.";
		return {};
	}
	std::array<wchar_t, 4096> path{};
	const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
	const auto tool = std::filesystem::path(path.data()).parent_path() / L"pixtool.exe";
	if (!length || length >= path.size() || !std::filesystem::exists(tool))
	{
		error = "The loaded PIX capturer has no matching native-open CLI; finalization cannot be confirmed.";
		return {};
	}
	return std::make_unique<PixCaptureAdapter>(tool);
#else
	error = "Official PIX capture headers were unavailable when this build was configured.";
	return {};
#endif
}

bool IsPixCaptureInstalled() noexcept
{
#if SPARKLE_RHI_WITH_PIX_CAPTURE
	if (GetModuleHandleW(L"WinPixGpuCapturer.dll"))
	{
		return true;
	}
	std::array<wchar_t, 4096> programFiles{};
	const DWORD length = GetEnvironmentVariableW(L"ProgramFiles", programFiles.data(), static_cast<DWORD>(programFiles.size()));
	if (!length || length >= programFiles.size())
	{
		return false;
	}
	std::error_code error;
	const auto root = std::filesystem::path(programFiles.data()) / L"Microsoft PIX";
	std::filesystem::directory_iterator versions(root, error), end;
	for (unsigned count = 0; !error && versions != end && count < 32; ++count, versions.increment(error))
	{
		std::error_code fileError;
		if (std::filesystem::is_regular_file(versions->path() / L"WinPixGpuCapturer.dll", fileError) && std::filesystem::is_regular_file(versions->path() / L"pixtool.exe", fileError))
		{
			return true;
		}
	}
#endif
	return false;
}
