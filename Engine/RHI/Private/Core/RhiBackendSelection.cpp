#include "PCH.h"

#include "Core/RhiBackendSelection.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Core/RhiBackendApi.h"

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

ERhiBackendApi ResolveBuildDefaultRhiBackendApi() noexcept
{
#if defined(SPARKLE_RHI_DEFAULT_BACKEND_VULKAN)
	return ERhiBackendApi::Vulkan;
#else
	return ERhiBackendApi::D3D12;
#endif
}

bool IsRhiBackendApiCompiled(ERhiBackendApi api) noexcept
{
	switch (api)
	{
		case ERhiBackendApi::D3D12:
			return SPARKLE_RHI_WITH_D3D12 != 0;
		case ERhiBackendApi::Vulkan:
			return SPARKLE_RHI_WITH_VULKAN != 0;
		case ERhiBackendApi::Unknown:
		default:
			return false;
	}
}
