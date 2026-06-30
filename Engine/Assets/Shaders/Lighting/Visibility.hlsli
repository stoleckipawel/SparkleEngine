#ifndef SPARKLE_VISIBILITY_HLSLI
#define SPARKLE_VISIBILITY_HLSLI

namespace LightingVisibility
{
	float ApplyVisibility(float radianceScale, float visibility)
	{
		return radianceScale * visibility;
	}
}

#endif
