#ifndef SPARKLE_SCENE_DEPTH_UTILS_HLSLI
#define SPARKLE_SCENE_DEPTH_UTILS_HLSLI

namespace SceneDepthUtils
{
	bool IsSkyDeviceZ(float deviceZ)
	{
		return !(deviceZ > 0.0f);
	}

	float LinearizeDeviceZ(float deviceZ, float nearZ, float farZ)
	{
		return IsSkyDeviceZ(deviceZ) ? farZ : nearZ / deviceZ;
	}

	float DeviceZFromLinearDepth(float sceneDepth, float nearZ)
	{
		return sceneDepth > 0.0f ? nearZ / sceneDepth : 0.0f;
	}

	float SkyDepth(float farZ)
	{
		return farZ;
	}

	bool IsSkyDepth(float sceneDepth, float farZ)
	{
		return sceneDepth >= farZ - max(1.0e-3f, farZ * 1.0e-6f);
	}
}

#endif
