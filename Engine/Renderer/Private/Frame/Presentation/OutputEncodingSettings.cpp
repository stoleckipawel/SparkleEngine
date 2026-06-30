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
	const EngineOutputColorEncoding sanitizedEncoding = SanitizeOutputColorEncoding(encoding);
	if (sanitizedEncoding == EngineOutputColorEncoding::Srgb)
	{
		return 1u;
	}
	if (sanitizedEncoding == EngineOutputColorEncoding::Linear)
	{
		return 0u;
	}

	switch (backBufferFormat)
	{
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return 1u;
		default:
			return 1u;
	}
}

OutputEncodingUniformData BuildOutputEncodingUniformDataFromCVars() noexcept
{
	return OutputEncodingUniformData{
	    .OutputColorEncoding = ResolveShaderOutputEncoding(CVarOutputColorEncoding.Get(), CVarBackBufferFormat.Get())};
}
