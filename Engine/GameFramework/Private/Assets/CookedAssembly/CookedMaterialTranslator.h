#pragma once

#include "Assets/Cooked/LoadedMaterialAsset.h"
#include "Scene/Materials/MaterialDesc.h"

#include <string>

namespace Assets
{
	class CookedMaterialTranslator final
	{
	  public:
		bool Translate(const LoadedMaterialAsset& materialAsset, MaterialDesc& outMaterialDesc, std::string& outErrorMessage) const;

	  private:
		static AlphaMode TranslateAlphaMode(CookedAlphaMode alphaMode) noexcept;
	};
}