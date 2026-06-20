#include "HostGraphicsCapabilities.h"

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#include <cwctype>
#include <string_view>

namespace SparkleLauncher
{
	namespace
	{
#if defined(_WIN32)
		bool WideContainsInsensitive(std::wstring_view text, std::wstring_view needle)
		{
			if (needle.empty() || needle.size() > text.size())
			{
				return false;
			}

			for (std::size_t offset = 0; offset + needle.size() <= text.size(); ++offset)
			{
				bool matches = true;
				for (std::size_t index = 0; index < needle.size(); ++index)
				{
					if (std::towupper(text[offset + index]) != std::towupper(needle[index]))
					{
						matches = false;
						break;
					}
				}
				if (matches)
				{
					return true;
				}
			}

			return false;
		}

		HostGraphicsVendor DetectVendorFromDeviceId(std::wstring_view deviceId)
		{
			if (WideContainsInsensitive(deviceId, L"VEN_10DE"))
			{
				return HostGraphicsVendor::Nvidia;
			}
			if (WideContainsInsensitive(deviceId, L"VEN_1002") || WideContainsInsensitive(deviceId, L"VEN_1022"))
			{
				return HostGraphicsVendor::Amd;
			}
			if (WideContainsInsensitive(deviceId, L"VEN_8086"))
			{
				return HostGraphicsVendor::Intel;
			}
			return HostGraphicsVendor::Other;
		}

		void MarkVendor(HostGraphicsCapabilities& capabilities, HostGraphicsVendor vendor, bool primaryAdapter)
		{
			switch (vendor)
			{
			case HostGraphicsVendor::Nvidia:
				capabilities.HasNvidiaAdapter = true;
				if (primaryAdapter)
				{
					capabilities.PrimaryVendor = HostGraphicsVendor::Nvidia;
				}
				return;
			case HostGraphicsVendor::Amd:
				capabilities.HasAmdAdapter = true;
				if (primaryAdapter)
				{
					capabilities.PrimaryVendor = HostGraphicsVendor::Amd;
				}
				return;
			case HostGraphicsVendor::Intel:
				capabilities.HasIntelAdapter = true;
				if (primaryAdapter)
				{
					capabilities.PrimaryVendor = HostGraphicsVendor::Intel;
				}
				return;
			case HostGraphicsVendor::Other:
				if (primaryAdapter)
				{
					capabilities.PrimaryVendor = HostGraphicsVendor::Other;
				}
				return;
			case HostGraphicsVendor::Unknown:
				return;
			}
		}
#endif

		HostGraphicsCapabilities DetectHostGraphicsCapabilities()
		{
			HostGraphicsCapabilities capabilities;

#if defined(_WIN32)
			for (DWORD deviceIndex = 0;; ++deviceIndex)
			{
				DISPLAY_DEVICEW adapter{};
				adapter.cb = sizeof(adapter);
				if (!EnumDisplayDevicesW(nullptr, deviceIndex, &adapter, 0))
				{
					break;
				}

				if ((adapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0)
				{
					continue;
				}

				const HostGraphicsVendor vendor = DetectVendorFromDeviceId(adapter.DeviceID);
				const bool primaryAdapter = (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;
				capabilities.AdapterDetected = true;
				MarkVendor(capabilities, vendor, primaryAdapter);
			}

			if (capabilities.PrimaryVendor == HostGraphicsVendor::Unknown)
			{
				if (capabilities.HasNvidiaAdapter)
				{
					capabilities.PrimaryVendor = HostGraphicsVendor::Nvidia;
				}
				else if (capabilities.HasAmdAdapter)
				{
					capabilities.PrimaryVendor = HostGraphicsVendor::Amd;
				}
				else if (capabilities.HasIntelAdapter)
				{
					capabilities.PrimaryVendor = HostGraphicsVendor::Intel;
				}
				else if (capabilities.AdapterDetected)
				{
					capabilities.PrimaryVendor = HostGraphicsVendor::Other;
				}
			}
#endif

			if (capabilities.HasNvidiaAdapter)
			{
				capabilities.Summary = capabilities.HasAmdAdapter ?
				                           "Detected NVIDIA and AMD graphics adapters; NVIDIA SDK dependencies stay enabled." :
				                           "Detected NVIDIA graphics adapter; NVIDIA SDK dependencies stay enabled.";
			}
			else if (capabilities.HasAmdAdapter)
			{
				capabilities.Summary = "Detected AMD graphics adapter; NVIDIA SDK dependencies stay disabled.";
			}
			else if (capabilities.HasIntelAdapter)
			{
				capabilities.Summary = "Detected Intel graphics adapter; NVIDIA SDK dependencies stay disabled.";
			}
			else if (capabilities.AdapterDetected)
			{
				capabilities.Summary = "Detected graphics adapter, but it does not identify as NVIDIA or AMD; NVIDIA SDK dependencies stay disabled.";
			}
			else
			{
				capabilities.Summary = "Graphics adapter vendor could not be detected; NVIDIA SDK dependencies stay disabled by default.";
			}

			return capabilities;
		}
	}

	const HostGraphicsCapabilities& GetHostGraphicsCapabilities()
	{
		static const HostGraphicsCapabilities capabilities = DetectHostGraphicsCapabilities();
		return capabilities;
	}
}
