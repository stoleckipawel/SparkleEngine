#pragma once

cbuffer FrameUniformData
{
	uint FrameIndex;
	float TotalTimeSeconds;
	float DeltaTimeSeconds;
	float ScaledTotalTimeSeconds;
	float ScaledDeltaTimeSeconds;
};
