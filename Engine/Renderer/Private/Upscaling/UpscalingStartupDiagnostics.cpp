#include "../PCH.h"

#include "Upscaling/UpscalingStartupDiagnostics.h"

#include "Upscaling/DlssCapabilityReport.h"

void LogUpscalingStartupDiagnostics(const RhiCapabilities& capabilities) noexcept
{
	const DlssCapabilityReport dlssCapabilities = DlssCapabilityReporter::Build(capabilities);
	DlssCapabilityReporter::LogOnce(dlssCapabilities);
}

