#pragma once

struct SceneLightingUniformData
{
	uint DirectionalLightCount;
	uint PointLightCount;
	uint SpotLightCount;
	uint RectLightCount;
};

ConstantBuffer<SceneLightingUniformData> SceneLighting;
