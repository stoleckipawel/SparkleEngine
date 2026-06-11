#include "CommonPS.hlsli"
#include "Debug/InstanceView.hlsli"
#include "MotionVector.hlsli"

struct GBufferOutput
{
	float4 BaseColor : SV_Target0;
	float4 Normal : SV_Target1;
	float4 Material : SV_Target2;
	float4 Emissive : SV_Target3;
	float4 Subsurface : SV_Target4;
	float DeviceZ : SV_Target5;
	float2 MotionVector : SV_Target6;
};

void main(in PS::Input Input, out GBufferOutput Output)
{
	PS::PrepareInput(Input);

	Material::Properties MatProps = Material::Sample(Input);
	MatProps.BaseColor = InstanceView::ApplyInstanceGroupVisualization(MatProps.BaseColor, Input.InstanceId);

	const float outputAlpha = (MatProps.AlphaMode == Material::AlphaModeBlend) ? MatProps.Alpha : 1.0f;
	Output.BaseColor = float4(MatProps.BaseColor, outputAlpha);
	Output.Normal = float4(normalize(MatProps.NormalWorld), 0.0f);
	Output.Material = float4(
	    saturate(MatProps.Metallic),
	    saturate(MatProps.Roughness),
	    saturate(MatProps.AmbientOcclusion),
	    saturate((float) MatProps.AlphaMode / 255.0f));
	Output.Emissive = float4(MatProps.Emissive, 0.0f);
	Output.Subsurface = float4(saturate(MatProps.SubsurfaceColor), saturate(MatProps.SubsurfaceStrength));
	Output.DeviceZ = Input.Position.z;

	Output.MotionVector = MotionVectors::Compute(
		Input.Position,
		Input.PrevClipPosition,
		ViewportSize);
}
