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

const char* UpscalerProviderFailureDomainToString(EUpscalerProviderFailureDomain domain) noexcept
{
	switch (domain)
	{
		case EUpscalerProviderFailureDomain::None:
			return "None";
		case EUpscalerProviderFailureDomain::Sdk:
			return "SDK";
		case EUpscalerProviderFailureDomain::Driver:
			return "Driver";
		case EUpscalerProviderFailureDomain::Backend:
			return "Backend";
		case EUpscalerProviderFailureDomain::Feature:
			return "Feature";
		case EUpscalerProviderFailureDomain::ResourceState:
			return "ResourceState";
		case EUpscalerProviderFailureDomain::InputContract:
			return "InputContract";
	}

	return "Unknown";
}
