#include "PCH.h"

#include "Settings/RenderDeviceBackBufferFormatResolver.h"

#include "RHI/Public/CVars/RHICVars.h"

PixelFormat ResolveRenderDeviceBackBufferFormatFromCVars() noexcept
{
	return CVarBackBufferFormat.Get();
}
