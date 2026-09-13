#ifndef SPARKLE_RAY_TRACING_PATH_BSDF_HLSLI
#define SPARKLE_RAY_TRACING_PATH_BSDF_HLSLI

#include "/Engine/Common/Constants.hlsli"
#include "/Engine/Common/Sampling.hlsli"
#include "/Engine/RayTracing/PathSurface.hlsli"
#include "/Engine/RayTracing/RayTracingPathSample.hlsli"

namespace PathBsdf
{
	struct LobeMasses
	{
		float Diffuse;
		float Specular;
		uint Count;
		bool SpecularDelta;
	};

	struct Evaluation
	{
		float3 F;
		float PdfW;
		float Cosine;
		bool HasSupport;
	};

	LobeMasses BuildEqualLobeMasses(RayTracingPathSurface surface)
	{
		LobeMasses result = (LobeMasses)0;
		const float3 f0 = lerp(surface.DielectricF0.xxx, surface.BaseColor, surface.Metallic);
		const bool diffuse = any((1.0f - surface.Metallic) * surface.BaseColor > 0.0f);
		const bool specular = any(f0 > 0.0f);
		result.Count = (diffuse ? 1u : 0u) + (specular ? 1u : 0u);
		if (result.Count != 0u)
		{
			const float mass = rcp((float)result.Count);
			result.Diffuse = diffuse ? mass : 0.0f;
			result.Specular = specular ? mass : 0.0f;
		}
		result.SpecularDelta = surface.Roughness == 0.0f;
		return result;
	}

	float3 FresnelSchlick(float cosine, float3 f0)
	{
		const float oneMinusCosine = 1.0f - cosine;
		const float oneMinusCosine2 = oneMinusCosine * oneMinusCosine;
		return f0 + (1.0f.xxx - f0) * oneMinusCosine2 * oneMinusCosine2 * oneMinusCosine;
	}

	float DistributionGGX(float noH, float alpha)
	{
		const float alpha2 = alpha * alpha;
		const float denominatorBase = noH * noH * (alpha2 - 1.0f) + 1.0f;
		const float denominator = PI * denominatorBase * denominatorBase;
		return denominator > 0.0f ? alpha2 / denominator : 0.0f;
	}

	float SmithG1(float noX, float alpha)
	{
		if (noX <= 0.0f)
		{
			return 0.0f;
		}
		const float noX2 = noX * noX;
		const float tanTheta2 = (1.0f - noX2) / noX2;
		return 2.0f / (1.0f + sqrt(1.0f + alpha * alpha * tanTheta2));
	}

	float ShadingNormalCorrection(RayTracingPathSurface surface, float3 directionWorld)
	{
		const float wiNs = dot(directionWorld, surface.NormalWorld);
		const float woNg = dot(surface.ViewDirWorld, surface.GeometricNormalWorld);
		const float wiNg = dot(directionWorld, surface.GeometricNormalWorld);
		const float woNs = dot(surface.ViewDirWorld, surface.NormalWorld);
		const float denominator = wiNg * woNs;
		return denominator != 0.0f ? abs((wiNs * woNg) / denominator) : 0.0f;
	}

	Evaluation EvaluateContinuous(RayTracingPathSurface surface, float3 directionWorld, LobeMasses masses)
	{
		Evaluation result = (Evaluation)0;
		const float noL = dot(surface.NormalWorld, directionWorld);
		const float noV = dot(surface.NormalWorld, surface.ViewDirWorld);
		const float ngL = dot(surface.GeometricNormalWorld, directionWorld);
		if (noL <= 0.0f || noV <= 0.0f || ngL <= 0.0f || masses.Count == 0u)
		{
			return result;
		}

		const float3 halfVectorUnnormalized = directionWorld + surface.ViewDirWorld;
		const float halfVectorLength2 = dot(halfVectorUnnormalized, halfVectorUnnormalized);
		if (halfVectorLength2 <= 0.0f)
		{
			return result;
		}
		const float3 halfVector = halfVectorUnnormalized * rsqrt(halfVectorLength2);
		const float noH = dot(surface.NormalWorld, halfVector);
		const float voH = dot(surface.ViewDirWorld, halfVector);
		if (noH <= 0.0f || voH <= 0.0f)
		{
			return result;
		}

		const float3 f0 = lerp(surface.DielectricF0.xxx, surface.BaseColor, surface.Metallic);
		const float3 fresnel = FresnelSchlick(voH, f0);
		const float3 diffuse = (1.0f.xxx - fresnel) * (1.0f - surface.Metallic) * surface.BaseColor * INV_PI;
		float3 specular = 0.0f.xxx;
		float specularPdfW = 0.0f;
		if (!masses.SpecularDelta && masses.Specular > 0.0f)
		{
			const float alpha = surface.Roughness * surface.Roughness;
			const float distribution = DistributionGGX(noH, alpha);
			const float g1View = SmithG1(noV, alpha);
			const float geometry = g1View * SmithG1(noL, alpha);
			specular = fresnel * distribution * geometry / (4.0f * noL * noV);
			specularPdfW = distribution * g1View / (4.0f * noV);
		}

		const float correction = ShadingNormalCorrection(surface, directionWorld);
		result.F = (diffuse + specular) * correction;
		result.PdfW = masses.Diffuse * noL * INV_PI + masses.Specular * specularPdfW;
		result.Cosine = ngL;
		result.HasSupport = result.PdfW > 0.0f && any(result.F > 0.0f);
		return result;
	}

	float3 SampleVisibleGGXHalfVector(RayTracingPathSurface surface, float2 sample)
	{
		float3 tangentWorld;
		float3 bitangentWorld;
		CommonSampling::BuildOrthonormalBasis(surface.NormalWorld, tangentWorld, bitangentWorld);
		const float alpha = surface.Roughness * surface.Roughness;
		const float3 localView = float3(dot(surface.ViewDirWorld, tangentWorld),
		                                dot(surface.ViewDirWorld, bitangentWorld),
		                                dot(surface.ViewDirWorld, surface.NormalWorld));
		const float3 stretchedView = normalize(float3(alpha * localView.xy, localView.z));
		const float length2 = dot(stretchedView.xy, stretchedView.xy);
		const float3 basis1 =
		    length2 > 0.0f ? float3(-stretchedView.y, stretchedView.x, 0.0f) * rsqrt(length2) : float3(1.0f, 0.0f, 0.0f);
		const float3 basis2 = cross(stretchedView, basis1);
		const float radius = sqrt(sample.x);
		const float phi = TWO_PI * sample.y;
		const float diskX = radius * cos(phi);
		const float originalDiskY = radius * sin(phi);
		const float blend = 0.5f * (1.0f + stretchedView.z);
		const float diskY = (1.0f - blend) * sqrt(max(0.0f, 1.0f - diskX * diskX)) + blend * originalDiskY;
		const float projectedZ = sqrt(max(0.0f, 1.0f - diskX * diskX - diskY * diskY));
		const float3 visibleNormal = diskX * basis1 + diskY * basis2 + projectedZ * stretchedView;
		const float3 localHalfVector = normalize(float3(alpha * visibleNormal.xy, max(visibleNormal.z, 0.0f)));
		return normalize(tangentWorld * localHalfVector.x + bitangentWorld * localHalfVector.y
		                 + surface.NormalWorld * localHalfVector.z);
	}

	RayTracingPathSample::DirectionSample Sample(RayTracingPathSurface surface,
	                                             LobeMasses masses,
	                                             uint selectedLobe,
	                                             float2 sample)
	{
		RayTracingPathSample::DirectionSample result = (RayTracingPathSample::DirectionSample)0;
		result.Lobe = selectedLobe;
		if (selectedLobe == RayTracingPathSample::LobeDiffuse)
		{
			const CommonSampling::CosineHemisphereSample direction =
			    CommonSampling::SampleCosineHemisphere(surface.NormalWorld, sample);
			result.DirectionWorld = direction.DirectionWorld;
		}
		else
		{
			if (masses.SpecularDelta)
			{
				result.DirectionWorld = normalize(reflect(-surface.ViewDirWorld, surface.NormalWorld));
				const float3 f0 = lerp(surface.DielectricF0.xxx, surface.BaseColor, surface.Metallic);
				const float cosine = abs(dot(surface.ViewDirWorld, surface.NormalWorld));
				const float correction = ShadingNormalCorrection(surface, result.DirectionWorld);
				result.Throughput = FresnelSchlick(cosine, f0) * correction / masses.Specular;
				result.Lobe = RayTracingPathSample::LobeSpecular;
				result.Delta = true;
				result.HasSupport = any(result.Throughput > 0.0f);
				return result;
			}
			const float3 halfVector = SampleVisibleGGXHalfVector(surface, sample);
			result.DirectionWorld = normalize(reflect(-surface.ViewDirWorld, halfVector));
		}
		const Evaluation evaluation = EvaluateContinuous(surface, result.DirectionWorld, masses);
		result.PdfW = evaluation.PdfW;
		result.Throughput = evaluation.HasSupport ? evaluation.F * evaluation.Cosine / evaluation.PdfW : 0.0f.xxx;
		result.Delta = false;
		result.HasSupport = evaluation.HasSupport;
		return result;
	}
}

#endif
