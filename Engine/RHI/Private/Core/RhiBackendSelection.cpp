#include "PCH.h"

#include "Core/RhiBackendSelection.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Core/RhiBackendApi.h"

#include <Windows.h>

#include <cstddef>
#include <cwctype>
#include <string>
#include <string_view>

const char* RhiBackendApiToString(ERhiBackendApi api) noexcept
{
	switch (api)
	{
		case ERhiBackendApi::D3D12:
			return "D3D12";
		case ERhiBackendApi::Vulkan:
			return "Vulkan";
		case ERhiBackendApi::Unknown:
		default:
			return "Unknown";
	}
}

bool TryParseRhiBackendApi(std::string_view value, ERhiBackendApi& outApi) noexcept
{
	if (Strings::EqualsIgnoreCase(value, "d3d12") || Strings::EqualsIgnoreCase(value, "dx12")
	    || Strings::EqualsIgnoreCase(value, "direct3d12"))
	{
		outApi = ERhiBackendApi::D3D12;
		return true;
	}

	if (Strings::EqualsIgnoreCase(value, "vulkan") || Strings::EqualsIgnoreCase(value, "vk"))
	{
		outApi = ERhiBackendApi::Vulkan;
		return true;
	}

	return false;
}

static std::wstring_view ReadRhiBackendCommandLineToken(std::wstring_view commandLine, std::size_t& offset) noexcept
{
	while (offset < commandLine.size() && std::iswspace(commandLine[offset]))
	{
		++offset;
	}

	if (offset >= commandLine.size())
	{
		return {};
	}

	const std::size_t tokenStart = offset;
	if (commandLine[offset] == L'"')
	{
		++offset;
		const std::size_t quotedStart = offset;
		while (offset < commandLine.size() && commandLine[offset] != L'"')
		{
			++offset;
		}
		const std::size_t quotedEnd = offset;
		if (offset < commandLine.size())
		{
			++offset;
		}
		return commandLine.substr(quotedStart, quotedEnd - quotedStart);
	}

	while (offset < commandLine.size() && !std::iswspace(commandLine[offset]))
	{
		++offset;
	}

	return commandLine.substr(tokenStart, offset - tokenStart);
}

static bool TryParseSelectionTokenValue(std::string_view value, ERhiBackendApi& outApi) noexcept
{
	if (TryParseRhiBackendApi(value, outApi))
	{
		return true;
	}

	outApi = ERhiBackendApi::Unknown;
	return true;
}

static bool TryResolveRhiBackendFromCommandLine(ERhiBackendApi& outApi) noexcept
{
	std::wstring_view commandLine{GetCommandLineW()};
	std::size_t offset = 0;
	while (offset < commandLine.size())
	{
		const std::wstring_view wideToken = ReadRhiBackendCommandLineToken(commandLine, offset);
		if (wideToken.empty())
		{
			continue;
		}

		const std::string token = Strings::ToNarrow(wideToken);
		constexpr std::string_view rendererEqualsPrefix = "--renderer=";
		constexpr std::string_view rhiEqualsPrefix = "--rhi=";
		constexpr std::string_view graphicsApiEqualsPrefix = "--graphics-api=";
		if (Strings::StartsWithIgnoreCase(token, rendererEqualsPrefix))
		{
			return TryParseSelectionTokenValue(std::string_view(token).substr(rendererEqualsPrefix.size()), outApi);
		}
		if (Strings::StartsWithIgnoreCase(token, rhiEqualsPrefix))
		{
			return TryParseSelectionTokenValue(std::string_view(token).substr(rhiEqualsPrefix.size()), outApi);
		}
		if (Strings::StartsWithIgnoreCase(token, graphicsApiEqualsPrefix))
		{
			return TryParseSelectionTokenValue(std::string_view(token).substr(graphicsApiEqualsPrefix.size()), outApi);
		}

		if (Strings::EqualsIgnoreCase(token, "--renderer") || Strings::EqualsIgnoreCase(token, "--rhi")
		    || Strings::EqualsIgnoreCase(token, "--graphics-api"))
		{
			const std::wstring_view wideValue = ReadRhiBackendCommandLineToken(commandLine, offset);
			return TryParseSelectionTokenValue(Strings::ToNarrow(wideValue), outApi);
		}
	}

	return false;
}

static ERhiBackendApi ResolveBuildDefaultRhiBackend() noexcept
{
#if defined(SPARKLE_RHI_DEFAULT_BACKEND_VULKAN)
	return ERhiBackendApi::Vulkan;
#else
	return ERhiBackendApi::D3D12;
#endif
}

ERhiBackendApi ResolveDefaultRhiBackendApi() noexcept
{
	ERhiBackendApi api = ResolveBuildDefaultRhiBackend();
	std::string configuredBackend;
	if (Environment::TryGetVariable("SPARKLE_RHI_BACKEND", configuredBackend) && !TryParseRhiBackendApi(configuredBackend, api))
	{
		api = ERhiBackendApi::Unknown;
	}

	TryResolveRhiBackendFromCommandLine(api);
	return api;
}
