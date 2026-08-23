#include "../../PCH.h"
#include "Passes/Presentation/OutputEncodingSettings.h"

#include "Passes/Presentation/OutputEncodingCVars.h"
#include "RHI/Public/CVars/RHICVars.h"

static const auto g_outputEncodingSettingsLogger = Logging::GetOrCreateLogger("Renderer.OutputEncodingSettings");

std::uint32_t ResolveShaderOutputEncoding(EngineOutputColorEncoding encoding, PixelFormat backBufferFormat) noexcept
{
	switch (encoding)
	{
		case EngineOutputColorEncoding::Srgb:
			return 1u;
		case EngineOutputColorEncoding::Linear:
			return 0u;
		case EngineOutputColorEncoding::Automatic:
			break;
		default:
			Diagnostics::Fatal(g_outputEncodingSettingsLogger, __FILE__, __LINE__, "Output settings contain an unknown color encoding.");
	}

	switch (backBufferFormat)
	{
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm:
			return 1u;
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return 0u;
		default:
			Diagnostics::Fatal(
			    g_outputEncodingSettingsLogger,
			    __FILE__,
			    __LINE__,
			    "Automatic output encoding received an unsupported back-buffer format.");
	}
}

OutputEncodingUniformData BuildOutputEncodingUniformData() noexcept
{
	return OutputEncodingUniformData{
	    .OutputColorEncoding = ResolveShaderOutputEncoding(CVarOutputColorEncoding.Get(), CVarBackBufferFormat.Get())};
}
