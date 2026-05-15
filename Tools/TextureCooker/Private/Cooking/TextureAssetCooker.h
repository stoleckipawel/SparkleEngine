#pragma once

#include "RHI/Private/D3D12/Textures/TextureLoadResult.h"
#include "TextureCookRequestList.h"

#include <dxgiformat.h>

#include <string>

	class TextureAssetCooker final
	{
	  public:
		bool Cook(const TextureCookRequest& request, std::string& outErrorMessage) const;
	};