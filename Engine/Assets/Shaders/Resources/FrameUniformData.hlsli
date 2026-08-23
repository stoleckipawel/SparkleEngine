#pragma once

cbuffer Frame
{
	uint FrameIndex;
	float TotalTimeSeconds;
	float DeltaTimeSeconds;
	float ScaledTotalTimeSeconds;
	float ScaledDeltaTimeSeconds;
};
