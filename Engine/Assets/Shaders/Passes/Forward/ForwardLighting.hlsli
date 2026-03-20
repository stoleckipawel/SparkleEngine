#include "Geometry/PixelInput.hlsli"
#include "Lighting/LightEvaluation.hlsli"

namespace Forward
{
	float3 CalculateLighting(
	    PS::Input psInput,
	    Material::Properties matProps,
	    out float3 outDirectDiffuse,
	    out float3 outDirectSubsurface,
	    out float3 outDirectSpecular,
	    out float3 outIndirectDiffuse,
	    out float3 outIndirectSpecular)
	{
		const float3 viewDir = normalize(Camera.Position - psInput.PositionWorld);

		Lighting::CalculateDirect(viewDir, matProps, outDirectDiffuse, outDirectSpecular, outDirectSubsurface);

		const float3 fakeAmbient = float3(0.00f, 0.0f, 0.00f);
		Lighting::CalculateIndirectIBL(viewDir, matProps, fakeAmbient, fakeAmbient, outIndirectDiffuse, outIndirectSpecular);

		return (outDirectDiffuse + outIndirectDiffuse) + (outDirectSpecular + outIndirectSpecular) + outDirectSubsurface +
		       matProps.Emissive;
	}
}  // namespace Forward
