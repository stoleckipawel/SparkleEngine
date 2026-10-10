#include "PCH.h"
#include "ExternalCapture/ExternalCaptureStartupPolicy.h"
#include "Core/Public/Process/CommandLineUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Renderer/Public/ExternalCapture/ExternalCaptureStartup.h"

#include <Windows.h>
#include <optional>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_externalCaptureLaunchLogger, "Application.ExternalCapture");

namespace ExternalCaptureStartupPolicy
{
	static std::optional<ExternalCaptureProvider> ParseAttachmentToken(
	    std::string_view token,
	    std::wstring_view commandLine,
	    std::size_t& offset)
	{
		if (Strings::EqualsIgnoreCase(token, "-AttachPix"))
		{
			return ExternalCaptureProvider::Pix;
		}
		if (Strings::EqualsIgnoreCase(token, "-AttachNSight"))
		{
			return ExternalCaptureProvider::NsightGraphics;
		}
		if (Strings::EqualsIgnoreCase(token, "-AttachRenderDoc"))
		{
			return ExternalCaptureProvider::RenderDoc;
		}

		constexpr std::string_view providerOption = "--capture-provider";
		if (Strings::EqualsIgnoreCase(token, providerOption) || Strings::StartsWithIgnoreCase(token, "--capture-provider="))
		{
			const std::string value = Strings::EqualsIgnoreCase(token, providerOption)
			    ? Strings::ToNarrow(CommandLine::ReadToken(commandLine, offset))
			    : std::string(token.substr(providerOption.size() + 1));

			ExternalCaptureProvider provider = ExternalCaptureProvider::None;
			if (!TryParseExternalCaptureProvider(value, provider))
			{
				Diagnostics::Fatal(
				    g_externalCaptureLaunchLogger,
				    __FILE__,
				    __LINE__,
				    "Invalid capture provider. Use none, nsight-graphics, pix or renderdoc.");
			}

			return provider;
		}

		if (Strings::StartsWithIgnoreCase(token, "-Attach"))
		{
			Diagnostics::Fatal(
			    g_externalCaptureLaunchLogger,
			    __FILE__,
			    __LINE__,
			    "Unknown capture attachment request. Use -AttachPix, -AttachNSight or -AttachRenderDoc.");
		}

		return std::nullopt;
	}

	static std::optional<ExternalCaptureProvider> ParseRequestedProvider()
	{
		const std::wstring_view commandLine{GetCommandLineW()};
		std::size_t offset = 0;
		std::optional<ExternalCaptureProvider> selected;
		while (offset < commandLine.size())
		{
			const std::string token = Strings::ToNarrow(CommandLine::ReadToken(commandLine, offset));
			const auto requested = ParseAttachmentToken(token, commandLine, offset);
			if (!requested.has_value())
			{
				continue;
			}

			if (selected.has_value())
			{
				Diagnostics::Fatal(
				    g_externalCaptureLaunchLogger,
				    __FILE__,
				    __LINE__,
				    "Select exactly one capture provider; duplicates and injected combinations are unverified.");
			}

			selected = requested;
		}

		return selected;
	}
}

ExternalCaptureProvider ResolveExternalCaptureStartupProvider() noexcept
{
	if (const auto requested = ExternalCaptureStartupPolicy::ParseRequestedProvider())
	{
		return *requested;
	}

	const auto provider = CVarExternalCaptureStartupProvider.Get();
	if (ExternalCaptureProviderToString(provider).empty())
	{
		Diagnostics::Fatal(g_externalCaptureLaunchLogger, __FILE__, __LINE__, "Invalid startup capture provider.");
	}

	return provider;
}
