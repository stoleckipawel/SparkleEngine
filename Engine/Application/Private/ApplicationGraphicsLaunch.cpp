#include "PCH.h"

#include "ApplicationGraphicsLaunch.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Process/CommandLineUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#if SPARKLE_WITH_EXTERNAL_CAPTURE
  #include "ExternalCapture/ExternalCaptureStartupPolicy.h"
#endif

#include <Windows.h>

#include <cstddef>
#include <string>
#include <string_view>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_applicationGraphicsLogger, "Application.Graphics");

namespace ApplicationGraphicsLaunch
{
	ERhiBackendApi ParseSelection(std::string_view value) noexcept
	{
		ERhiBackendApi api = ERhiBackendApi::Unknown;
		TryParseRhiBackendApi(value, api);
		return api;
	}

	bool TryResolveCommandLineSelection(ERhiBackendApi& api) noexcept
	{
		const std::wstring_view commandLine{GetCommandLineW()};
		std::size_t offset = 0;
		while (offset < commandLine.size())
		{
			const std::wstring_view wideToken = CommandLine::ReadToken(commandLine, offset);
			if (wideToken.empty())
			{
				continue;
			}

			const std::string token = Strings::ToNarrow(wideToken);
			constexpr std::string_view prefixes[]{"--renderer=", "--rhi=", "--graphics-api="};
			for (const std::string_view prefix : prefixes)
			{
				if (Strings::StartsWithIgnoreCase(token, prefix))
				{
					api = ParseSelection(std::string_view(token).substr(prefix.size()));
					return true;
				}
			}

			if (Strings::EqualsIgnoreCase(token, "--renderer") || Strings::EqualsIgnoreCase(token, "--rhi")
			    || Strings::EqualsIgnoreCase(token, "--graphics-api"))
			{
				api = ParseSelection(Strings::ToNarrow(CommandLine::ReadToken(commandLine, offset)));
				return true;
			}
		}
		return false;
	}
}

RendererGraphicsLaunch ResolveApplicationGraphicsLaunch() noexcept
{
	ERhiBackendApi api = ResolveBuildDefaultRhiBackendApi();
	std::string configuredBackend;
	if (Environment::TryGetVariable("SPARKLE_RHI_BACKEND", configuredBackend))
	{
		api = ApplicationGraphicsLaunch::ParseSelection(configuredBackend);
	}
	ApplicationGraphicsLaunch::TryResolveCommandLineSelection(api);
	if (api == ERhiBackendApi::Unknown)
	{
		Diagnostics::Fatal(
		    g_applicationGraphicsLogger,
		    __FILE__,
		    __LINE__,
		    "Invalid graphics backend selection. Use D3D12 or Vulkan with SPARKLE_RHI_BACKEND or --graphics-api.");
	}
	if (!IsRhiBackendApiCompiled(api))
	{
		Diagnostics::Fatal(
		    g_applicationGraphicsLogger,
		    __FILE__,
		    __LINE__,
		    std::string("Graphics backend '") + RhiBackendApiToString(api) + "' is not compiled into this product.");
	}

	RendererGraphicsLaunch launch{.BackendApi = api};
#if SPARKLE_WITH_EXTERNAL_CAPTURE
	launch.CaptureProvider = ResolveExternalCaptureStartupProvider();
#endif
	return launch;
}
