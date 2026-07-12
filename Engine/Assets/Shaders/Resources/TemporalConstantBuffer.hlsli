#pragma once

cbuffer PerTemporalConstantBufferData
{
	row_major float4x4 PrevViewMTX;
	row_major float4x4 PrevProjectionMTX;
	row_major float4x4 PrevViewProjMTX;
	float2 JitterCurrent;
	float2 JitterPrevious;
	uint HistoryValid;
	float4 _pad0;
	float4 _pad1;
};
