#pragma once

#ifdef SPARKLE_RHI_EXPORTS
	#define SPARKLE_RHI_API __declspec(dllexport)
#else
	#define SPARKLE_RHI_API __declspec(dllimport)
#endif

#ifdef SPARKLE_STATIC
	#undef SPARKLE_RHI_API
	#define SPARKLE_RHI_API
#endif
