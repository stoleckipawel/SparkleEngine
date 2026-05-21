#pragma once

#include "AssetCookerTypes.h"

#include <cstdint>

struct AssetCookRequest
{
	AssetCookerCategory category;
	const char* projectName;
	const char* configuration;
};

struct AssetRecookAsset
{
	AssetCookerCategory category;
	const char* sourcePath;
	const char* assetId;
};

struct AssetRecookRequest
{
	const char* projectName;
	const char* configuration;
	const AssetRecookAsset* assets;
	std::uint32_t assetCount;
};
