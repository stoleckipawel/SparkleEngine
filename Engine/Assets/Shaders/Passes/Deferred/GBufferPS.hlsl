#include "CommonPS.hlsli"
#include "Debug/InstanceView.hlsli"
#include "Passes/Deferred/GBufferPacking.hlsli"
#include "MotionVector.hlsli"

struct GBufferOutput
{
	float4 BaseColor : SV_Target0;
	float4 Normal : SV_Target1;
	float4 Material : SV_Target2;
	float4 Emissive : SV_Target3;
	float4 Subsurface : SV_Target4;
	float2 MotionVector : SV_Target5;
};

void main(in PS::Input Input, out GBufferOutput Output)
{
	PS::PrepareInput(Input);

	Material::Properties MatProps = Material::Sample(Input);
	MatProps.BaseColor = InstanceView::ApplyInstanceVisualization(MatProps.BaseColor, Input.GpuSceneSlot);

	Output.BaseColor = GBufferPacking::PackBaseColor(MatProps.BaseColor, MatProps.Alpha, MatProps.AlphaMode, Material::AlphaModeBlend);
	Output.Normal = GBufferPacking::PackNormal(MatProps.NormalWorld);
	Output.Material = GBufferPacking::PackMaterial(
	    MatProps.Metallic,
	    MatProps.Roughness,
	    MatProps.AmbientOcclusion,
	    MatProps.DielectricF0);
	Output.Emissive = GBufferPacking::PackEmissive(MatProps.Emissive);
	Output.Subsurface = GBufferPacking::PackSubsurface(MatProps.SubsurfaceColor, MatProps.SubsurfaceStrength);

	Output.MotionVector = MotionVectors::ComputeRaster(Input.Position.xy, Input.PrevClipPosition, ViewportSize);
}
