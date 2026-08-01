#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Interop/RhiInterposerHooks.h"

struct RendererBackendConfiguration final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	RhiInterposerHooks InterposerHooks;
};
