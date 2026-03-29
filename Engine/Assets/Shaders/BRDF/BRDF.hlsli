#pragma once

#include "BRDF/ShadingData.hlsli"
#include "BRDF/Fresnel.hlsli"
#include "BRDF/Distribution.hlsli"
#include "BRDF/Geometry.hlsli"
#include "BRDF/Diffuse.hlsli"
#include "BRDF/Subsurface.hlsli"
#include "BRDF/Specular.hlsli"
#include "BRDF/Occlusion.hlsli"

namespace BRDF
{
	namespace Direct
	{
		void Evaluate(
		    ShadingData sd,
		    float3 albedo,
		    float roughness,
		    float metallic,
		    float3 F0,
		    float3 subsurfaceColor,
		    float subsurfaceStrength,
		    out float3 outDiffuse,
		    out float3 outSpecular,
		    out float3 outSubsurface)
		{
			const float3 F = Fresnel::EvaluateDirect(sd.VoH, F0);

			outSpecular = Specular::EvaluateDirect(sd, roughness, F);

			const float3 kD = (1.0f - F) * (1.0f - metallic);
			outDiffuse = Diffuse::EvaluateDirect(albedo, roughness, sd) * kD;

			outSubsurface = Subsurface::EvaluateDirect(albedo, subsurfaceColor, roughness, subsurfaceStrength, sd) * (1.0f - metallic);
		}
	}  // namespace Direct


	namespace Indirect
	{
		void Evaluate(
		    float NoV,
		    float3 albedo,
		    float roughness,
		    float metallic,
		    float3 F0,
		    float3 irradiance,
		    float3 prefilteredEnv,
		    float ambientOcclusion,
		    out float3 outDiffuse,
		    out float3 outSpecular)
		{
			const float3 F = Fresnel::EvaluateIndirect(NoV, F0, roughness);


			outSpecular = Specular::EvaluateIndirect(NoV, F0, roughness, prefilteredEnv);


			const float3 kD = (1.0f - F) * (1.0f - metallic);
			outDiffuse = Diffuse::EvaluateIndirect(albedo) * irradiance * kD;


			outDiffuse *= Occlusion::MultibounceAO(ambientOcclusion, albedo);
			outSpecular *= Occlusion::SpecularOcclusion(NoV, ambientOcclusion, roughness);
		}
	}  // namespace Indirect

}  // namespace BRDF
