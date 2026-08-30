#include "PCH.h"

#include "Memory/RhiMemoryTypes.h"

std::string_view RhiMemoryCategoryName(RhiMemoryCategory category) noexcept
{
	switch (category)
	{
		case RhiMemoryCategory::Texture:
			return "Texture";
		case RhiMemoryCategory::Mesh:
			return "Mesh";
		case RhiMemoryCategory::RayTracing:
			return "RayTracing";
		case RhiMemoryCategory::TransientResource:
			return "TransientResource";
		case RhiMemoryCategory::Upload:
			return "Upload";
		case RhiMemoryCategory::Readback:
			return "Readback";
		case RhiMemoryCategory::ConstantBuffer:
			return "ConstantBuffer";
		case RhiMemoryCategory::Other:
		default:
			return "Other";
	}
}

std::string_view RhiMemoryResidencyClassName(RhiMemoryResidencyClass residencyClass) noexcept
{
	switch (residencyClass)
	{
		case RhiMemoryResidencyClass::DeviceLocal:
			return "DeviceLocal";
		case RhiMemoryResidencyClass::HostUpload:
			return "HostUpload";
		case RhiMemoryResidencyClass::HostReadback:
			return "HostReadback";
		case RhiMemoryResidencyClass::Transient:
			return "Transient";
		default:
			return "Unknown";
	}
}
