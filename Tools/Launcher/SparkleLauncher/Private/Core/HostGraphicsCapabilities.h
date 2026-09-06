#pragma once

#include <cstdint>
#include <string>

namespace SparkleLauncher
{
	enum class HostGraphicsVendor : std::uint8_t
	{
		Unknown,
		Nvidia,
		Amd,
		Intel,
		Other
	};

	struct HostGraphicsCapabilities
	{
		HostGraphicsVendor PrimaryVendor = HostGraphicsVendor::Unknown;
		bool HasNvidiaAdapter = false;
		bool HasAmdAdapter = false;
		bool HasIntelAdapter = false;
		bool AdapterDetected = false;
		std::string Summary;
	};

	const HostGraphicsCapabilities& GetHostGraphicsCapabilities();
}
