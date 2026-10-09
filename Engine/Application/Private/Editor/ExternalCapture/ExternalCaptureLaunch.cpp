#include "PCH.h"
#include "Editor/ExternalCapture/ExternalCaptureLaunch.h"

#if SPARKLE_APPLICATION_WITH_EXTERNAL_CAPTURE_UI
  #include "Editor/Public/ExternalCapture/ExternalCaptureToolbar.h"

  #include "Core/Public/Diagnostics/Verify.h"
  #include "Core/Public/Process/CommandLineUtils.h"
  #include "Core/Public/Strings/StringUtils.h"

  #include <Windows.h>

  #include <string>
  #include <string_view>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_editorCaptureLaunchLogger, "Application.Capture");

static ExternalCaptureToolRequests ResolveCaptureToolRequests() noexcept
{
	ExternalCaptureToolRequests requested;
	const std::wstring_view commandLine{GetCommandLineW()};
	std::size_t offset = 0;
	while (offset < commandLine.size())
	{
		const std::string token = Strings::ToNarrow(CommandLine::ReadToken(commandLine, offset));
		bool* selection = nullptr;
		if (Strings::EqualsIgnoreCase(token, "-AttachPix"))
		{
			selection = &requested.Pix;
		}
		else if (Strings::EqualsIgnoreCase(token, "-AttachNSight"))
		{
			selection = &requested.Nsight;
		}
		else if (Strings::EqualsIgnoreCase(token, "-AttachRenderDoc"))
		{
			selection = &requested.RenderDoc;
		}
		else if (Strings::StartsWithIgnoreCase(token, "-Attach"))
		{
			Diagnostics::Fatal(
			    g_editorCaptureLaunchLogger,
			    __FILE__,
			    __LINE__,
			    "Unknown capture attachment request. Use -AttachPix, -AttachNSight or -AttachRenderDoc.");
		}
		if (selection != nullptr)
		{
			if (*selection)
			{
				Diagnostics::Fatal(g_editorCaptureLaunchLogger, __FILE__, __LINE__, "Duplicate external capture attachment request.");
			}
			*selection = true;
		}
	}
	if (requested.Pix || requested.Nsight || requested.RenderDoc)
	{
		SPDLOG_LOGGER_WARN(
		    g_editorCaptureLaunchLogger.GetLogger(),
		    "Capture attachment requested: PIX={}, Nsight={}, RenderDoc={}. Native engine adapters are unavailable; "
		    "viewport controls show setup guidance. No capture tool has been loaded by these flags.",
		    requested.Pix,
		    requested.Nsight,
		    requested.RenderDoc);
	}
	return requested;
}
#endif

std::unique_ptr<ViewportToolbarActions> CreateRequestedCaptureToolbarActions()
{
#if SPARKLE_APPLICATION_WITH_EXTERNAL_CAPTURE_UI
	static const ExternalCaptureToolRequests requested = ResolveCaptureToolRequests();
	return CreateExternalCaptureToolbarActions(requested);
#else
	return {};
#endif
}
