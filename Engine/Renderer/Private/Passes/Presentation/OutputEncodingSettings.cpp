#include "../../PCH.h"
#include "Passes/Presentation/OutputEncodingSettings.h"

#include "Passes/Presentation/OutputEncodingCVars.h"
#include "RHI/Public/CVars/RHICVars.h"

namespace OutputEncodingSettings
{
	static const auto g_logger = Logging::GetOrCreateLogger("Renderer.OutputEncodingSettings");

	std::uint32_t ResolveShaderValue() noexcept
	{
		switch (CVarOutputColorEncoding.Get())
		{
			case EngineOutputColorEncoding::Srgb:
				return 1u;
			case EngineOutputColorEncoding::Linear:
				return 0u;
			case EngineOutputColorEncoding::Automatic:
				break;
			default:
				Diagnostics::Fatal(g_logger, __FILE__, __LINE__, "Output settings contain an unknown color encoding.");
		}

		const PixelFormat backBufferFormat = CVarBackBufferFormat.Get();
		const PixelFormat linearFormat = PixelFormatToLinear(backBufferFormat);
		if (linearFormat != PixelFormat::R8G8B8A8_UNorm && linearFormat != PixelFormat::B8G8R8A8_UNorm)
		{
			Diagnostics::Fatal(
			    g_logger,
			    __FILE__,
			    __LINE__,
			    "Automatic output encoding received an unsupported back-buffer format.");
		}
		return IsSrgbPixelFormat(backBufferFormat) ? 0u : 1u;
	}
}

OutputEncodingUniformData BuildOutputEncodingUniformData() noexcept
{
	return OutputEncodingUniformData{
	    .OutputColorEncoding = OutputEncodingSettings::ResolveShaderValue()};
}
