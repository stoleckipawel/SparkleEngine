#pragma once

#if defined(_WIN32) && defined(ASSET_COOKER_BUILDING_DLL)
	#define ASSET_COOKER_API __declspec(dllexport)
#elif defined(_WIN32) && !defined(ASSET_COOKER_BUILDING_STATIC)
	#define ASSET_COOKER_API __declspec(dllimport)
#else
	#define ASSET_COOKER_API
#endif
