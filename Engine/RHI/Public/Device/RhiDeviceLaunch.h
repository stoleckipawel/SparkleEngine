#pragma once

#include "../Core/RhiBackendApi.h"
#include "../Interop/RhiInterposerHooks.h"
class RhiExternalCapture;

struct RhiDeviceLaunch final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	RhiInterposerHooks InterposerHooks;
#if SPARKLE_TARGET_EDITOR && !SPARKLE_BUILD_SHIPPING
	RhiExternalCapture* ExternalCapture = nullptr;
#endif
};
