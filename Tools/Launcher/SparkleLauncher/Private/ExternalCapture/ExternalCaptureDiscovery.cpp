#include "ExternalCaptureDiscovery.h"
#include "HostGraphicsCapabilities.h"
#include "Core/Public/Environment/EnvironmentVariables.h"

#include <filesystem>

namespace SparkleLauncher
{
	static constexpr unsigned MaxCaptureInstallationEntries = 64;

	static bool IsInstalledToolFile(const std::filesystem::path& path)
	{
		std::error_code error;
		return std::filesystem::is_regular_file(path, error);
	}

	static std::filesystem::path FindToolDirectory(const std::filesystem::path& parent, std::string_view prefix, const std::filesystem::path& library, const std::filesystem::path& viewer)
	{
		std::error_code error;
		std::filesystem::directory_iterator directory(parent, error), end;
		for (unsigned visited = 0; !error && directory != end && visited < MaxCaptureInstallationEntries; directory.increment(error), ++visited)
		{
			const auto path = directory->path();
			if (path.filename().string().starts_with(prefix) && IsInstalledToolFile(path / library) && IsInstalledToolFile(path / viewer))
			{
				return path;
			}
		}

		return {};
	}

	static std::filesystem::path FindInstalledCaptureTool(ExternalCaptureProvider provider)
	{
		std::string value;
		if (!Environment::TryGetVariable("ProgramW6432", value) && !Environment::TryGetVariable("ProgramFiles", value))
		{
			return {};
		}

		const auto programFiles = std::filesystem::u8path(value);
		switch (provider)
		{
			case ExternalCaptureProvider::Pix:
				return FindToolDirectory(programFiles / "Microsoft PIX", "", "WinPixGpuCapturer.dll", "pixtool.exe");
			case ExternalCaptureProvider::NsightGraphics:

				return FindToolDirectory(
				    programFiles / "NVIDIA Corporation",
				    "Nsight Graphics ",
				    "target/windows-desktop-nomad-x64/ngfx-capture-injection.dll",
				    "host/windows-desktop-nomad-x64/ngfx-ui.exe");

			case ExternalCaptureProvider::RenderDoc:
			{
				const auto directory = programFiles / "RenderDoc";

				return IsInstalledToolFile(directory / "renderdoc.dll") && IsInstalledToolFile(directory / "qrenderdoc.exe") ? directory : std::filesystem::path{};
			}

			default:
				return {};
		}
	}

	static std::string_view GetUnsupportedCaptureReason(ExternalCaptureProvider provider, std::string_view graphicsApi, std::string_view productProfile)
	{
		if (productProfile != "DebugEditor" && productProfile != "DevelopmentEditor")
		{
			return "Capture is available in Debug and Development Editor.";
		}
		if (graphicsApi != "d3d12" && graphicsApi != "vulkan")
		{
			return "Select D3D12 or Vulkan.";
		}
		if (graphicsApi == "vulkan" && provider != ExternalCaptureProvider::RenderDoc)
		{
			return "This capture route is currently available on D3D12.";
		}
		if (provider == ExternalCaptureProvider::NsightGraphics && !GetHostGraphicsCapabilities().HasNvidiaAdapter)
		{
			return "Nsight Graphics requires an NVIDIA GPU.";
		}

		return {};
	}

	ExternalCaptureAvailability InspectExternalCaptureProvider(ExternalCaptureProvider provider, std::string_view graphicsApi, std::string_view productProfile)
	{
		if (provider == ExternalCaptureProvider::None)
		{
			return {true, true, "Launch without capture hooks."};
		}
		if (ExternalCaptureProviderToString(provider).empty())
		{
			return {false, false, "Unknown capture provider."};
		}

		const auto path = FindInstalledCaptureTool(provider);
		const bool installed = !path.empty();
		const auto unsupportedReason = GetUnsupportedCaptureReason(provider, graphicsApi, productProfile);
		if (!unsupportedReason.empty())
		{
			return {installed, false, std::string(unsupportedReason)};
		}

		return {installed, true, installed ? path.string() : "Install " + std::string(ExternalCaptureProviderDisplayName(provider)) + "."};
	}
}
