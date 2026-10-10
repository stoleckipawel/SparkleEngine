#pragma once

#include "RHI/Public/Diagnostics/ExternalCaptureProvider.h"
#include "Core/Public/Console/CVar.h"
#include "RendererAPI.h"
#if SPARKLE_TARGET_EDITOR && !SPARKLE_BUILD_SHIPPING
extern SPARKLE_RENDERER_API ConsoleVariable<ExternalCaptureProvider> CVarExternalCaptureStartupProvider;
#endif
