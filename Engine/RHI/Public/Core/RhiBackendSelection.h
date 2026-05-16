#pragma once

#include "../RHIAPI.h"
#include "RhiBackendApi.h"

#include <string_view>

struct RhiBackendSelection final
{
	ERhiBackendApi Api = ERhiBackendApi::D3D12;
};

SPARKLE_RHI_API const char* RhiBackendApiToString(ERhiBackendApi api) noexcept;
SPARKLE_RHI_API bool TryParseRhiBackendApi(std::string_view value, ERhiBackendApi& outApi) noexcept;
SPARKLE_RHI_API RhiBackendSelection ResolveDefaultRhiBackendSelection() noexcept;