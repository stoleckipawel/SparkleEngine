#include "../../PCH.h"
#include "Frame/Presentation/OutputEncodingSettings.h"

#include "Frame/Presentation/ToneMappingCVars.h"
#include "RHI/Public/CVars/RHICVars.h"

EngineOutputColorEncoding SanitizeOutputColorEncoding(EngineOutputColorEncoding encoding) noexcept
{
	switch (encoding)
	{
		case EngineOutputColorEncoding::Automatic:
		case EngineOutputColorEncoding::Linear:
		case EngineOutputColorEncoding::Srgb:
			return encoding;
		default:
			return EngineOutputColorEncoding::Automatic;
	}
}

std::uint32_t ResolveShaderOutputEncoding(EngineOutputColorEncoding encoding, PixelFormat backBufferFormat) noexcept
{
	(void)backBufferFormat;
	const EngineOutputColorEncoding sanitizedEncoding = SanitizeOutputColorEncoding(encoding);
	if (sanitizedEncoding == EngineOutputColorEncoding::Srgb)
	{
		return 1u;
	}
	if (sanitizedEncoding == EngineOutputColorEncoding::Linear)
	{
		return 0u;
	}

	return 1u;
}

OutputEncodingUniformData BuildOutputEncodingUniformDataFromCVars() noexcept
{
	return OutputEncodingUniformData{
	    .OutputColorEncoding = ResolveShaderOutputEncoding(CVarOutputColorEncoding.Get(), CVarBackBufferFormat.Get())};
}
