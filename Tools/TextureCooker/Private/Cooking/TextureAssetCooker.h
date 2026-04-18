#pragma once

#include "D3D12/Textures/TextureLoadResult.h"
#include "TextureCookRequestList.h"

#include <dxgiformat.h>

#include <string>

namespace Engine::AssetAuthoring
{
	class TextureAssetCooker final
	{
	  public:
		bool Cook(const TextureCookRequest& request, std::string& outErrorMessage) const;

	  private:
		static bool ResolveCookedTextureFormat(
		    DXGI_FORMAT sourceDxgiFormat,
		    TextureColorSpace colorSpace,
		    DXGI_FORMAT& outCookedDxgiFormat,
		    TextureFormatIntent& outFormatIntent,
		    std::string& outErrorMessage);
	};
}