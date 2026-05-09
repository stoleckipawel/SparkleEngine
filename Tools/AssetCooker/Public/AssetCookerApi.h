#pragma once

#include "AssetCookRequest.h"
#include "AssetCookResult.h"
#include "AssetCookerDll.h"
#include "AssetCookerTypes.h"

#include <cstdint>

struct AssetCookerContext;

extern "C"
{
	ASSET_COOKER_API AssetCookerContext* AssetCookerCreateContext(const AssetCookerConfig* config);
	ASSET_COOKER_API void AssetCookerDestroyContext(AssetCookerContext* context);
	ASSET_COOKER_API int AssetCookerCookProject(
	    AssetCookerContext* context,
	    const AssetCookRequest* request,
	    AssetCookResult* outResult);
	ASSET_COOKER_API int AssetCookerRecookAssets(
	    AssetCookerContext* context,
	    const AssetRecookRequest* request,
	    AssetCookResult* outResult);
	ASSET_COOKER_API int AssetCookerQueryCapabilities(
	    AssetCookerContext* context,
	    AssetCookerCapabilities* outCapabilities);
	ASSET_COOKER_API std::uint32_t AssetCookerGetLastDiagnostics(
	    AssetCookerContext* context,
	    const AssetCookDiagnostic** outDiagnostics);
}
