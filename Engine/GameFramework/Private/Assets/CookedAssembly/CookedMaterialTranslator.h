#pragma once

#include "Assets/Cooked/LoadedMaterialAsset.h"
#include "Scene/Materials/MaterialDesc.h"

namespace Assets
{
	class CookedMaterialTranslator final
	{
	  public:
		MaterialDesc Translate(const LoadedMaterialAsset& materialAsset) const;

	  private:
		static AlphaMode TranslateAlphaMode(CookedAlphaMode alphaMode);
	};
}
