#include "../PCH.h"

#include "Upscaling/UpscalingStartupDiagnostics.h"

#include "Upscaling/NvidiaDlss/DlssCapabilityReport.h"
#include "Upscaling/UpscalerSettings.h"

void LogUpscalingStartupDiagnostics(const RhiCapabilities& capabilities) noexcept
{
	const UpscalerSettings settings = BuildUpscalerSettingsFromCVars();
	if (settings.RequestedProvider != EUpscalerProviderKind::NvidiaDlss)
	{
		return;
	}

	const DlssCapabilityReport dlssCapabilities = DlssCapabilityReporter::Build(capabilities);
	DlssCapabilityReporter::LogOnce(dlssCapabilities);
}
