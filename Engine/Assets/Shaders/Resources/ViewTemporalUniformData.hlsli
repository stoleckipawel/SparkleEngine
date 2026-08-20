#pragma once

cbuffer ViewTemporalUniformData
{
	row_major float4x4 PreviousWorldToViewMatrix;
	row_major float4x4 PreviousViewToClipMatrix;
	row_major float4x4 PreviousWorldToClipMatrix;
	float2 CurrentJitterNdc;
	float2 PreviousJitterNdc;
	uint HistoryValid;
};
