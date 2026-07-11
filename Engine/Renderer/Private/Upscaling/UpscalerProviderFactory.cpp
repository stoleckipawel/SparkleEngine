#include "../PCH.h"
#include "Upscaling/UpscalerProviderFactory.h"

#include "Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"

std::unique_ptr<IUpscalerProvider> CreateConfiguredUpscalerProvider()
{
	switch (CVarUpscalerProvider.Get())
	{
		case EUpscalerProviderKind::NvidiaDlss:
			return std::make_unique<NvidiaDlssUpscalerProvider>();
		case EUpscalerProviderKind::Linear:
		default:
			return {};
	}
}
