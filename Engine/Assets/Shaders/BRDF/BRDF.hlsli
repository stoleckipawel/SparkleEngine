#pragma once

#include "/Engine/BRDF/ShadingData.hlsli"
#include "/Engine/BRDF/Fresnel.hlsli"
#include "/Engine/BRDF/Distribution.hlsli"
#include "/Engine/BRDF/Geometry.hlsli"
#include "/Engine/BRDF/Diffuse.hlsli"
#include "/Engine/BRDF/Subsurface.hlsli"
#include "/Engine/BRDF/Specular.hlsli"
#include "/Engine/BRDF/Occlusion.hlsli"

namespace BRDF
{
	namespace Direct
	{
		struct Response
		{
			float3 Diffuse;
			float3 Specular;
			float3 Subsurface;
		};

		Response Evaluate(ShadingData sd,
		                  float3 albedo,
		                  float roughness,
		                  float metallic,
		                  float3 F0,
		                  float3 subsurfaceColor,
		                  float subsurfaceStrength,
		                  bool evaluateDiffuse,
		                  bool evaluateSpecular,
		                  bool evaluateSubsurface)
		{
			Response response = (Response)0;
			const float3 F = Fresnel::EvaluateDirect(sd.VoH, F0);

			if (evaluateSpecular)
			{
				response.Specular = Specular::EvaluateDirect(sd, roughness, F);
			}

			const float3 kD = (1.0f - F) * (1.0f - metallic);
			const float subsurfaceWeight = saturate(subsurfaceStrength);
			if (evaluateDiffuse)
			{
				response.Diffuse = Diffuse::EvaluateDirect(albedo, roughness, sd) * kD * (1.0f - subsurfaceWeight);
			}

			if (evaluateSubsurface && subsurfaceWeight > 0.0f)
			{
				response.Subsurface =
				    Subsurface::EvaluateDirect(albedo, subsurfaceColor, roughness, subsurfaceWeight, sd) * kD * subsurfaceWeight;
			}
			return response;
		}
	}
	namespace Indirect
	{
		void Evaluate(float NoV,
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
	}
}
