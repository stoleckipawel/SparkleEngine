#include "PCH.h"

#include "ShaderData/SkyUniformData.h"

SkyUniformData MakeSkyUniformData(const RenderSkyData& sky) noexcept
{
	return SkyUniformData{
	    .Color = sky.color,
	    .Intensity = sky.intensity,
	    .Enabled = sky.enabled ? 1u : 0u};
}
