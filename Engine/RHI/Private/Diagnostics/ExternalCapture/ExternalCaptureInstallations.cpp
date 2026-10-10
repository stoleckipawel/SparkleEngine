#include "PCH.h"
#include "Diagnostics/ExternalCapture/ExternalCaptureAdapter.h"

bool HasInstalledExternalCaptureProvider() noexcept
{
	static const bool installed = []
	{
		if (IsRenderDocCaptureInstalled())
		{
			return true;
		}
#if SPARKLE_RHI_WITH_D3D12
		return IsPixCaptureInstalled() || IsNsightCaptureInstalled();
#else
		return false;
#endif
	}();

	return installed;
}
