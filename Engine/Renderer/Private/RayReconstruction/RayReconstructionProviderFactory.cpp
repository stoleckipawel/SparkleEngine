#include "../PCH.h"
#include "RayReconstruction/RayReconstructionProviderFactory.h"

#include "RayReconstruction/NvidiaDlssRayReconstruction/NvidiaDlssRayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"

std::unique_ptr<IRayReconstructionProvider> CreateConfiguredRayReconstructionProvider()
{
	switch (CVarRayReconstructionMode.Get())
	{
		case EngineRayReconstructionMode::NvidiaDlssRayReconstruction:
			return std::make_unique<NvidiaDlssRayReconstructionProvider>();
		case EngineRayReconstructionMode::Off:
		default:
			return {};
	}
}
