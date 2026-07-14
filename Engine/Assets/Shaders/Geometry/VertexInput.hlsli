#pragma once

namespace VS
{
	struct Input
	{
		float3 Position : POSITION;
		float2 TexCoord : TEXCOORD;
		float3 Normal : NORMAL;
		float4 Tangent : TANGENT;
		uint VertexId : SV_VertexID;
		uint InstanceId : SV_InstanceID;
	};
}  // namespace VS
