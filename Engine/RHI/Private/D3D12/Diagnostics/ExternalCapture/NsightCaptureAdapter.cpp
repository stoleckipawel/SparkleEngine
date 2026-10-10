#include "PCH.h"
#include "Diagnostics/ExternalCapture/ExternalCaptureAdapter.h"
#include "Core/Public/Paths/ProductUserStatePaths.h"
#include "NsightCaptureViewer.h"

#include <Windows.h>
#include <array>

#if SPARKLE_RHI_WITH_NSIGHT_CAPTURE
  #include <NGFX_GraphicsCapture_D3D12.h>

static void* LoadNsightLibrary(const NGFX_PathChar* path)
{
	// The SDK supplies absolute installed-library paths; no bare-name search.
	if (!path || !std::filesystem::path(path).is_absolute())
	{
		return nullptr;
	}

	return LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}

class NsightInstallations final
{
public:
	NsightInstallations() { m_result = NGFX_EnumerateInstallations(m_installations.data(), static_cast<std::uint32_t>(m_installations.size()), &m_count); }

	~NsightInstallations() noexcept { NGFX_FreeInstallations(m_installations.data(), m_count); }

	NsightInstallations(const NsightInstallations&) = delete;
	NsightInstallations& operator=(const NsightInstallations&) = delete;

	bool IsAvailable() const noexcept { return m_result == NGFX_Result_Success && m_count != 0; }

	const NGFX_PathChar* GetFirstPath() const noexcept { return m_installations[0].installationPath; }

private:
	std::array<NGFX_InstallationInfo, 8> m_installations{};

	std::uint32_t m_count = 0;
	NGFX_Result m_result = NGFX_Result_Success;
};

class NsightCaptureAdapter final : public ExternalCaptureAdapter
{
public:
	bool Begin(const ExternalCaptureNativeTarget&, const std::filesystem::path&, std::string& error) override
	{
		NGFX_ArtifactFileCount_Params count{};
		count.version = NGFX_ArtifactFileCount_Params_VER;
		if (NGFX_GraphicsCapture_GetCaptureFileCount(&count) != NGFX_Result_Success)
		{
			error = "Nsight could not enumerate its completed captures.";
			return false;
		}
		m_captureIndex = count.count;
		NGFX_GraphicsCapture_RequestCapture_D3D12_Params request{};
		request.version = NGFX_GraphicsCapture_RequestCapture_D3D12_Params_VER;
		request.delimiter = NGFX_GraphicsCapture_Delimiter_Present;
		request.framesToCapture = 1;
		if (NGFX_GraphicsCapture_RequestCapture_D3D12(&request) != NGFX_Result_Success)
		{
			error = "Nsight rejected the Graphics Capture request (native activity may be busy).";
			return false;
		}
		return true;
	}

	void End(const ExternalCaptureNativeTarget&) noexcept override {}

	ExternalCaptureNativeResult Poll() override
	{
		NGFX_ArtifactFileCount_Params count{};
		count.version = NGFX_ArtifactFileCount_Params_VER;
		if (NGFX_GraphicsCapture_GetCaptureFileCount(&count) != NGFX_Result_Success || count.count <= m_captureIndex)
		{
			return {};
		}
		if (count.count != m_captureIndex + 1)
		{
			return {.State = ExternalCaptureState::Quarantined, .Message = "Nsight completed multiple captures; request attribution is ambiguous."};
		}
		std::array<NGFX_PathChar, 4096> path{};
		NGFX_ArtifactFilePath_Params artifact{};
		artifact.version = NGFX_ArtifactFilePath_Params_VER;
		artifact.artifactIndex = m_captureIndex;
		artifact.filePath = path.data();
		artifact.filePathCapacity = static_cast<std::uint32_t>(path.size());
		if (NGFX_GraphicsCapture_GetCaptureFilePath(&artifact) != NGFX_Result_Success)
		{
			return {.State = ExternalCaptureState::Failed, .Message = "Nsight completed capture but did not provide a finalized artifact path."};
		}
		const bool opened = OpenNsightCaptureInDesktopShell(path.data());

		return {
		    .State = ExternalCaptureState::Completed,
		    .Artifact = std::filesystem::path(path.data()),
		    .Message = opened ? "Experimental SDK: capture finalized and handed to Nsight." : "Experimental SDK: capture finalized; open the artifact in Nsight Graphics."};
	}

private:
	std::uint32_t m_captureIndex = 0;
};
#endif

#if SPARKLE_RHI_WITH_NSIGHT_CAPTURE
static bool InjectNsightGraphicsCapture(std::string& error)
{
	NGFX_SetLibraryLoadFn(&LoadNsightLibrary);
	const NsightInstallations installations;
	if (!installations.IsAvailable())
	{
		error = "No Nsight Graphics installation was discovered by the installed NGFX SDK.";
		return false;
	}

	NGFX_GraphicsCapture_InjectionSettings settings{};
	if (NGFX_GraphicsCapture_InjectionSettings_SetDefaults(&settings) != NGFX_Result_Success)
	{
		error = "Nsight could not initialize its official injection defaults.";
		return false;
	}

	const auto root = Filesystem::GetProductUserStatePaths().CapturesRoot / "ExternalCapture" / ("Nsight-" + std::to_string(GetCurrentProcessId()));

	std::error_code ioError;
	std::filesystem::create_directories(root, ioError);
	const std::string output = root.string();
	settings.noHUD = true;
	settings.outputDir = output.c_str();

	NGFX_GraphicsCapture_Inject_D3D12_Params inject{};
	inject.version = NGFX_GraphicsCapture_Inject_D3D12_Params_VER;
	inject.installationPath = installations.GetFirstPath();
	inject.settings = &settings;
	const NGFX_Result injected = ioError ? NGFX_Result_InvalidParameter : NGFX_GraphicsCapture_Inject_D3D12(&inject);
	if (injected != NGFX_Result_Success)
	{
		error = "Nsight Graphics Capture injection failed: " + std::to_string(static_cast<int>(injected));
		return false;
	}

	return true;
}

static bool InitializeNsightGraphicsCapture(std::string& error)
{
	NGFX_GraphicsCapture_InitializeActivity_D3D12_Params initialize{};
	initialize.version = NGFX_GraphicsCapture_InitializeActivity_D3D12_Params_VER;
	if (NGFX_GraphicsCapture_InitializeActivity_D3D12(&initialize) != NGFX_Result_Success)
	{
		error = "Nsight injected hooks but activity initialization failed. Restart the Editor.";
		return false;
	}

	return true;
}
#endif

std::unique_ptr<ExternalCaptureAdapter> CreateNsightCaptureAdapter(std::string& error)
{
#if SPARKLE_RHI_WITH_NSIGHT_CAPTURE
	if (!InjectNsightGraphicsCapture(error) || !InitializeNsightGraphicsCapture(error))
	{
		return {};
	}

	return std::make_unique<NsightCaptureAdapter>();
#else
	error = "The pinned NGFX 0.9.2 headers were unavailable when this build was configured.";
	return {};
#endif
}

bool IsNsightCaptureInstalled() noexcept
{
#if SPARKLE_RHI_WITH_NSIGHT_CAPTURE
	const NsightInstallations installations;
	return installations.IsAvailable();
#else
	return false;
#endif
}
