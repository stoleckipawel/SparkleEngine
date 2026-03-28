#include "CommonVS.hlsli"

struct ShadowVSOutput
{
	float4 Position : SV_POSITION;
	float Depth : TEXCOORD0;
};

void main(in VS::Input Input, out ShadowVSOutput Output)
{
	const float4 positionWorld = PositionLocalToWorld(float4(Input.Position, 1.0f));
	const float4 positionClip = PositionWorldToClip(positionWorld);

	Output.Position = positionClip;
	Output.Depth = positionClip.z / positionClip.w;
}