// =============================================================================
// Forward Lit Pixel Shader
// =============================================================================
// PBR forward lighting with material sampling and debug view modes.

#include "CommonPS.hlsli"

void main(in PS::Input Input, out PS::Output Output)
{
	PS::PrepareInput(Input);

	// Sample all material properties
	Material::Properties MatProps = Material::Sample(Input);

	// Calculate Forward Lighting
	float3 DirectDiffuse;
	float3 DirectSubsurface;
	float3 DirectSpecular;
	float3 IndirectDiffuse;
	float3 IndirectSpecular;
	float3 Lit =
	    Forward::CalculateLighting(Input, MatProps, DirectDiffuse, DirectSubsurface, DirectSpecular, IndirectDiffuse, IndirectSpecular);

	// Postprocess Final Output
	const float3 FinalColor =
	    ViewMode::Resolve(Lit, MatProps, DirectDiffuse, DirectSpecular, DirectSubsurface, IndirectDiffuse, IndirectSpecular);
	const float outputAlpha = (MatProps.AlphaMode == Material::AlphaModeBlend) ? MatProps.Alpha : 1.0f;
	Output.Color0 = float4(FinalColor, outputAlpha);
}
