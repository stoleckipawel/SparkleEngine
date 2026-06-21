#include "PCH.h"

#include "Settings/RenderDeviceSettingsResolver.h"

#include "RHI/Public/CVars/RHICVars.h"

RenderDeviceSettings BuildRenderDeviceSettingsFromCVars() noexcept
{
	return RenderDeviceSettings{
	    .BackBufferFormat = CVarBackBufferFormat.Get()};
}

