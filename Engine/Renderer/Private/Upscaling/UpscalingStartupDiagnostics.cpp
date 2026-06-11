#include "../PCH.h"

#include "Upscaling/UpscalingStartupDiagnostics.h"

#include "Upscaling/NvidiaDlss/DlssCapabilityReport.h"

void LogUpscalingStartupDiagnostics(const RhiCapabilities& capabilities) noexcept
{
	const DlssCapabilityReport dlssCapabilities = DlssCapabilityReporter::Build(capabilities);
	DlssCapabilityReporter::LogOnce(dlssCapabilities);
}
