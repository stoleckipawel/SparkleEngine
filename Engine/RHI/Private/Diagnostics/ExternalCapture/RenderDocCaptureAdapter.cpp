#include "PCH.h"
#include "Diagnostics/ExternalCapture/ExternalCaptureAdapter.h"

#include <Windows.h>
#include <array>

#if SPARKLE_RHI_WITH_RENDERDOC_CAPTURE
  #include <renderdoc_app.h>

static std::filesystem::path GetInstalledRenderDocLibraryPath()
{
	std::array<wchar_t, 4096> programFiles{};
	const DWORD length = GetEnvironmentVariableW(L"ProgramFiles", programFiles.data(), static_cast<DWORD>(programFiles.size()));

	return length && length < programFiles.size() ? std::filesystem::path(programFiles.data()) / L"RenderDoc" / L"renderdoc.dll"
	                                              : std::filesystem::path{};
}

class RenderDocCaptureAdapter final : public ExternalCaptureAdapter
{
public:
	explicit RenderDocCaptureAdapter(RENDERDOC_API_1_6_0& api) :
	    m_api(api)
	{
	}

	bool Begin(const ExternalCaptureNativeTarget& target, const std::filesystem::path& path, std::string& error) override
	{
		if (m_api.IsFrameCapturing())
		{
			error = "RenderDoc is already capturing through a native control.";
			return false;
		}
		m_firstCapture = m_api.GetNumCaptures();
		const auto utf8 = path.u8string();
		const std::string filename(utf8.begin(), utf8.end());
		m_api.SetCaptureFilePathTemplate(filename.c_str());
		m_api.SetActiveWindow(target.Root, target.Window);
		m_api.StartFrameCapture(target.Root, target.Window);
		m_ended = false;

		if (!m_api.IsFrameCapturing())
		{
			error = "RenderDoc did not start capture for the bound API root and host window.";
			return false;
		}
		return true;
	}

	void End(const ExternalCaptureNativeTarget& target) noexcept override
	{
		m_ended = m_api.EndFrameCapture(target.Root, target.Window) == 1;
	}

	ExternalCaptureNativeResult Poll() override
	{
		if (!m_ended)
		{
			return {
			    .State = m_api.IsFrameCapturing() ? ExternalCaptureState::Quarantined : ExternalCaptureState::Failed,
			    .Message = "RenderDoc failed to end the requested native interval."};
		}
		if (m_api.GetNumCaptures() != m_firstCapture + 1)
		{
			return {
			    .State = ExternalCaptureState::Quarantined,
			    .Message = "RenderDoc artifact count is ambiguous; restart before another request."};
		}
		std::filesystem::path artifact;
		if (!TryGetCaptureArtifact(artifact))
		{
			return {.State = ExternalCaptureState::Failed, .Message = "RenderDoc completed but its artifact path could not be retrieved."};
		}

		return OpenCaptureInReplay(artifact);
	}

private:
	bool TryGetCaptureArtifact(std::filesystem::path& artifact) const
	{
		std::array<char, 4096> path{};
		std::uint32_t length = static_cast<std::uint32_t>(path.size());
		std::uint64_t timestamp = 0;
		if (!m_api.GetCapture(m_firstCapture, nullptr, &length, &timestamp) || length > path.size()
		    || !m_api.GetCapture(m_firstCapture, path.data(), &length, &timestamp))
		{
			return false;
		}

		artifact = std::filesystem::u8path(path.data());
		return true;
	}

	ExternalCaptureNativeResult OpenCaptureInReplay(const std::filesystem::path& artifact) const
	{
		const auto utf8 = artifact.u8string();
		const std::string replayArguments = "\"" + std::string(utf8.begin(), utf8.end()) + "\"";
		const bool opened = m_api.LaunchReplayUI(0, replayArguments.c_str()) != 0;

		return {
		    .State = ExternalCaptureState::Completed,
		    .Artifact = artifact,
		    .Message = opened ? "Capture finalized and handed to RenderDoc."
		                      : "Capture finalized; RenderDoc replay UI launch failed. Open the artifact manually."};
	}

	RENDERDOC_API_1_6_0& m_api;
	std::uint32_t m_firstCapture = 0;
	bool m_ended = false;
};
#endif

std::unique_ptr<ExternalCaptureAdapter> CreateRenderDocCaptureAdapter(std::string& error)
{
#if SPARKLE_RHI_WITH_RENDERDOC_CAPTURE
	HMODULE module = GetModuleHandleW(L"renderdoc.dll");
	if (!module)
	{
		const auto path = GetInstalledRenderDocLibraryPath();
		if (path.empty())
		{
			error = "Cannot resolve the installed RenderDoc directory.";
			return {};
		}
		module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	}
	if (!module)
	{
		error = "RenderDoc is not installed at its standard location or could not be loaded.";
		return {};
	}
	// Hooked modules stay loaded through process exit. Never FreeLibrary on recovery.
	const auto getApi = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(module, "RENDERDOC_GetAPI"));
	RENDERDOC_API_1_6_0* api = nullptr;
	if (!getApi || getApi(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&api)) != 1 || !api)
	{
		error = "Loaded RenderDoc does not expose the required application API. Restart with a compatible installation.";
		return {};
	}
	api->SetCaptureKeys(nullptr, 0);
	api->SetFocusToggleKeys(nullptr, 0);
	return std::make_unique<RenderDocCaptureAdapter>(*api);
#else
	error = "RenderDoc application headers were unavailable when this build was configured.";
	return {};
#endif
}

bool IsRenderDocCaptureInstalled() noexcept
{
#if SPARKLE_RHI_WITH_RENDERDOC_CAPTURE
	if (GetModuleHandleW(L"renderdoc.dll"))
	{
		return true;
	}
	std::error_code error;
	return std::filesystem::is_regular_file(GetInstalledRenderDocLibraryPath(), error);
#else
	return false;
#endif
}
