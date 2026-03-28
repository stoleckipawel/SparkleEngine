#include "CommonPS.hlsli"

void main(in PS::Input Input, out PS::Output Output)
{
	PS::PrepareInput(Input);


	Material::Properties MatProps = Material::Sample(Input);


	float3 DirectDiffuse;
	float3 DirectSubsurface;
	float3 DirectSpecular;
	float3 Lit = Lighting::Evaluate(Input, MatProps, DirectDiffuse, DirectSubsurface, DirectSpecular);


	const float3 FinalColor = ViewMode::Resolve(Lit, MatProps, DirectDiffuse, DirectSpecular, DirectSubsurface);

	const float outputAlpha = (MatProps.AlphaMode == Material::AlphaModeBlend) ? MatProps.Alpha : 1.0f;
	Output.Color0 = float4(FinalColor, outputAlpha);
}

