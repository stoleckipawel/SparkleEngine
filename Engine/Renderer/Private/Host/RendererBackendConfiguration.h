#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Interop/RhiExternalFeatureHooks.h"

struct RendererBackendConfiguration final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	RhiExternalFeatureHooks ExternalFeatureHooks;
};
