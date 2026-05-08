#pragma once

#include "RHI/Public/D3D12/Textures/TextureLoadResult.h"
#include "TextureCookRequestList.h"

#include <dxgiformat.h>

#include <string>

namespace AssetAuthoring
{
	class TextureAssetCooker final
	{
	  public:
		bool Cook(const TextureCookRequest& request, std::string& outErrorMessage) const;
	};
}