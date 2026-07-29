#pragma once

cbuffer PerTemporalConstantBufferData
{
	row_major float4x4 PreviousWorldToViewMatrix;
	row_major float4x4 PreviousViewToClipMatrix;
	row_major float4x4 PreviousWorldToClipMatrix;
	float2 CurrentJitterNdc;
	float2 PreviousJitterNdc;
	uint HistoryValid;
	float4 _pad0;
	float4 _pad1;
};
