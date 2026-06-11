#include "../PCH.h"
#include "Upscaling/UpscalerProvider.h"

const char* UpscalerProviderKindToString(EUpscalerProviderKind kind) noexcept
{
	switch (kind)
	{
		case EUpscalerProviderKind::Passthrough:
			return "Passthrough";
		case EUpscalerProviderKind::NvidiaDlss:
			return "NvidiaDLSS";
	}

	return "Unknown";
}

const char* UpscalerProviderStatusToString(EUpscalerProviderStatus status) noexcept
{
	switch (status)
	{
		case EUpscalerProviderStatus::Unavailable:
			return "Unavailable";
		case EUpscalerProviderStatus::Available:
			return "Available";
		case EUpscalerProviderStatus::Active:
			return "Active";
		case EUpscalerProviderStatus::FailedWithFallback:
			return "FailedWithFallback";
	}

	return "Unknown";
}
