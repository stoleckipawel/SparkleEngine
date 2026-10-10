#pragma once

#include "RHI/Public/Diagnostics/ExternalCaptureProvider.h"
#include <string>
#include <string_view>

namespace SparkleLauncher
{
	struct ExternalCaptureAvailability final
	{
		bool Installed = false;
		bool Supported = false;
		std::string Detail;

		bool Available() const noexcept { return Installed && Supported; }
	};

	ExternalCaptureAvailability InspectExternalCaptureProvider(
	    ExternalCaptureProvider provider,
	    std::string_view graphicsApi,
	    std::string_view productProfile);
}
