#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Interop/RhiD3D12InterposerHooks.h"

struct RendererBackendConfiguration final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	RhiD3D12InterposerHooks D3D12InterposerHooks;
};
