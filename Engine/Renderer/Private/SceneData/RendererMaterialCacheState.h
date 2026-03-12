#pragma once

#include "Assets/MaterialDesc.h"
#include "D3D12DescriptorHandle.h"
#include "Renderer/Public/SceneData/MaterialData.h"

#include <vector>

struct RendererMaterialCacheState
{
	std::vector<MaterialDesc> cachedMaterialDescs;
	std::vector<MaterialData> cachedMaterialData;
	std::vector<D3D12DescriptorHandle> materialTextureTables;
	bool materialCacheBuilt = false;
	bool materialCacheUsesLoadedMaterials = false;
};