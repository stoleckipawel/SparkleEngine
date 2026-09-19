#include "../../../PCH.h"
#include "Passes/Presentation/Display/ToneMappingSettings.h"

static const auto g_toneMappingSettingsLogger = Logging::GetOrCreateLogger("Renderer.ToneMappingSettings");

static std::uint32_t ToShaderToneMapper(EngineToneMapper toneMapper) noexcept
{
	switch (toneMapper)
	{
		case EngineToneMapper::Reinhard:
			return 0u;
		case EngineToneMapper::AcesApprox:
			return 1u;
		case EngineToneMapper::AcesFilmic:
			return 2u;
		default:
			Diagnostics::Fatal(g_toneMappingSettingsLogger, __FILE__, __LINE__, "Tone-mapping settings contain an unknown tone mapper.");
	}
}

ToneMappingUniformData BuildToneMappingUniformData(EngineToneMapper toneMapper) noexcept
{
	return ToneMappingUniformData{.ToneMapper = ToShaderToneMapper(toneMapper)};
}
