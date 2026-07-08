#include "../PCH.h"
#include "RayReconstruction/RayReconstructionProvider.h"

const char* RayReconstructionProviderKindToString(ERayReconstructionProviderKind kind) noexcept
{
	switch (kind)
	{
		case ERayReconstructionProviderKind::NvidiaDlrr:
			return "NVIDIA DLRR";
		case ERayReconstructionProviderKind::None:
		default:
			return "None";
	}
}

const char* RayReconstructionProviderFailureDomainToString(ERayReconstructionProviderFailureDomain domain) noexcept
{
	switch (domain)
	{
		case ERayReconstructionProviderFailureDomain::Sdk:
			return "SDK";
		case ERayReconstructionProviderFailureDomain::Driver:
			return "Driver";
		case ERayReconstructionProviderFailureDomain::Backend:
			return "Backend";
		case ERayReconstructionProviderFailureDomain::Feature:
			return "Feature";
		case ERayReconstructionProviderFailureDomain::ResourceState:
			return "ResourceState";
		case ERayReconstructionProviderFailureDomain::InputContract:
			return "InputContract";
		case ERayReconstructionProviderFailureDomain::None:
		default:
			return "None";
	}
}
