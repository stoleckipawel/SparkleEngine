#pragma once

#include <array>
#include <cstdint>
#include <string_view>

enum class ExternalCaptureProvider : std::uint8_t
{
	None,
	NsightGraphics,
	Pix,
	RenderDoc
};

inline constexpr std::array ExternalCaptureProviders{
    ExternalCaptureProvider::None,
    ExternalCaptureProvider::NsightGraphics,
    ExternalCaptureProvider::Pix,
    ExternalCaptureProvider::RenderDoc};

constexpr std::string_view ExternalCaptureProviderToString(ExternalCaptureProvider provider) noexcept
{
	switch (provider)
	{
		case ExternalCaptureProvider::None:
			return "none";
		case ExternalCaptureProvider::NsightGraphics:
			return "nsight-graphics";
		case ExternalCaptureProvider::Pix:
			return "pix";
		case ExternalCaptureProvider::RenderDoc:
			return "renderdoc";
	}
	return {};
}

constexpr std::string_view ExternalCaptureProviderDisplayName(ExternalCaptureProvider provider) noexcept
{
	switch (provider)
	{
		case ExternalCaptureProvider::None:
			return "None";
		case ExternalCaptureProvider::NsightGraphics:
			return "Nsight Graphics";
		case ExternalCaptureProvider::Pix:
			return "PIX";
		case ExternalCaptureProvider::RenderDoc:
			return "RenderDoc";
	}
	return {};
}

constexpr bool TryParseExternalCaptureProvider(std::string_view value, ExternalCaptureProvider& provider) noexcept
{
	for (auto candidate : ExternalCaptureProviders)
	{
		if (ExternalCaptureProviderToString(candidate) == value)
		{
			provider = candidate;
			return true;
		}
	}
	return false;
}
