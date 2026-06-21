#pragma once

#include "../../../Core/Public/Console/CVar.h"
#include "../Formats/PixelFormat.h"
#include "../RHIAPI.h"

extern SPARKLE_RHI_API ConsoleVariable<bool> CVarVSync;
extern SPARKLE_RHI_API ConsoleVariable<bool> CVarPreferHighPerformanceAdapter;
extern SPARKLE_RHI_API ConsoleVariable<bool> CVarRayTracingPreferPartitionedTlas;
extern SPARKLE_RHI_API ConsoleVariable<PixelFormat> CVarBackBufferFormat;
