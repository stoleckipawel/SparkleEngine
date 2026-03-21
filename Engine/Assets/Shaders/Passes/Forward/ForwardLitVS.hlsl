




#include "CommonVS.hlsli"

void main(in VS::Input Input, out VS::Output Output)
{

	const float4 positionWorld = PositionLocalToWorld(float4(Input.Position, 1.0f));
	const float3 normalWorld = NormalLocalToWorld(Input.Normal);
	const float4 tangentWorld = TangentLocalToWorld(Input.Tangent);


	const float3 bitangentWorld = ComputeBitangent(normalWorld, tangentWorld);


	const float4 positionClip = PositionWorldToClip(positionWorld);


	Output.Position = positionClip;
	Output.PositionWorld = positionWorld.xyz;
	Output.NormalWorld = normalWorld;
	Output.TangentWorld = tangentWorld;
	Output.BitangentWorld = bitangentWorld;
	Output.TexCoord = Input.TexCoord;
	Output.Color = Input.Color;
}
