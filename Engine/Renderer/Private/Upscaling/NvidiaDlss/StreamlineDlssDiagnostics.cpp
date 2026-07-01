#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"

const char* DlssProviderRuntimeStateToString(EDlssProviderRuntimeState state) noexcept
{
	switch (state)
	{
		case EDlssProviderRuntimeState::NotSelected:
			return "NotSelected";
		case EDlssProviderRuntimeState::Unavailable:
			return "Unavailable";
		case EDlssProviderRuntimeState::AvailableNotCreated:
			return "AvailableNotCreated";
		case EDlssProviderRuntimeState::Created:
			return "Created";
		case EDlssProviderRuntimeState::Evaluating:
			return "Evaluating";
		case EDlssProviderRuntimeState::FailedWithFallback:
			return "FailedWithFallback";
	}

	return "Unknown";
}

const char* DlssFeatureKindToString(EDlssFeatureKind feature) noexcept
{
	switch (feature)
	{
		case EDlssFeatureKind::SuperResolution:
			return "SuperResolution";
		case EDlssFeatureKind::NativeAA:
			return "NativeAA";
	}

	return "Unknown";
}

const char* DlssFeatureStateToString(EDlssFeatureState state) noexcept
{
	switch (state)
	{
		case EDlssFeatureState::NotSelected:
			return "NotSelected";
		case EDlssFeatureState::Unavailable:
			return "Unavailable";
		case EDlssFeatureState::Available:
			return "Available";
		case EDlssFeatureState::Enabled:
			return "Enabled";
		case EDlssFeatureState::Active:
			return "Active";
		case EDlssFeatureState::FailedWithFallback:
			return "FailedWithFallback";
	}

	return "Unknown";
}
