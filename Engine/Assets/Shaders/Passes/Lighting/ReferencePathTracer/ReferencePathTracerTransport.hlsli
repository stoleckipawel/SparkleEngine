#ifndef SPARKLE_REFERENCE_PATH_TRACER_TRANSPORT_HLSLI
#define SPARKLE_REFERENCE_PATH_TRACER_TRANSPORT_HLSLI

#include "/Engine/RayTracing/PathBsdf.hlsli"
#include "/Engine/RayTracing/PathTracer.hlsli"
#include "/Engine/RayTracing/RayEndpoints.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialHit.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTrace.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerDirectLighting.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerSampler.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerUniformData.hlsli"

namespace ReferencePathTracer
{
	RayTracingPathSample::DirectionSample SamplePathDirection(RayTracingPathSurface surface,
	                                                          PathBsdf::LobeMasses lobeMasses,
	                                                          SampleIdentity sampleIdentity,
	                                                          uint surfaceDepth)
	{
		const CommonRandom::CategoricalSample lobeChoice =
		    CommonRandom::SampleCategorical(RandomWord(sampleIdentity, SurfaceDimension(surfaceDepth, LobeChoiceOffset)), lobeMasses.Count);

		uint selectedLobe = RayTracingPathSample::LobeSpecular;
		if (lobeMasses.Diffuse > 0.0f && lobeChoice.Index == 0u)
		{
			selectedLobe = RayTracingPathSample::LobeDiffuse;
		}

		const float2 bsdfSample =
		    float2(CommonRandom::OpenUnitInterval(RandomWord(sampleIdentity, SurfaceDimension(surfaceDepth, BsdfDirectionXOffset))),
		           CommonRandom::OpenUnitInterval(RandomWord(sampleIdentity, SurfaceDimension(surfaceDepth, BsdfDirectionYOffset))));
		return PathBsdf::Sample(surface, lobeMasses, selectedLobe, bsdfSample);
	}

	bool SurvivesRussianRoulette(inout float3 throughput, SampleIdentity sampleIdentity, uint surfaceVertexCount)
	{
		const float targetSurvival = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05f, 0.95f);
		const float scaledSurvival = targetSurvival * 16777216.0f;
		const uint lowerThreshold = (uint)floor(scaledSurvival);
		const float thresholdFraction = scaledSurvival - (float)lowerThreshold;
		const uint roundedThreshold =
		    lowerThreshold + (thresholdFraction > 0.5f || (thresholdFraction == 0.5f && (lowerThreshold & 1u) != 0u) ? 1u : 0u);
		const uint threshold = min(max(roundedThreshold, 1u), 16777215u);
		const uint rouletteValue = RandomWord(sampleIdentity, SurfaceDimension(surfaceVertexCount - 1u, RouletteOffset)) >> 8u;

		if (rouletteValue >= threshold)
		{
			return false;
		}

		PathTracer::ApplySurvivalCompensation(throughput, (float)threshold * 0x1.0p-24f);
		return true;
	}

	float3 TraceSurfaceTransport(RaytracingAccelerationStructure sceneTlas,
	                             Texture2D skyTexture,
	                             SamplerState skySampler,
	                             RayEndpoints::Ray traversal,
	                             SampleIdentity sampleIdentity)
	{
		const LightCounts lightCounts = GetLightCounts();
		PathTracer::PathState path;
		path.OriginWorld = traversal.Origin;
		path.DirectionWorld = traversal.Direction;
		path.Throughput = 1.0f.xxx;
		path.SurfaceDepth = 0u;

		PreviousPathEvent previousEvent;
		previousEvent.PositionWorld = 0.0f.xxx;
		previousEvent.BsdfPdfW = 0.0f;
		previousEvent.Delta = true;

		float3 contribution = 0.0f.xxx;

		[loop]
		for (;;)
		{
			const RayTracingTraceResult trace = TraceSceneRay(sceneTlas,
			                                                  path.OriginWorld,
			                                                  path.DirectionWorld,
			                                                  traversal.TMin,
			                                                  traversal.TMax,
			                                                  RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
			                                                  0xFFu);

			if (!trace.Hit)
			{
				PathTracer::AddRadiance(
				    contribution,
				    path.Throughput,
				    EvaluateEnvironmentRadiance(lightCounts, previousEvent, path.DirectionWorld, skyTexture, skySampler));
				break;
			}

			const RayTracingHitSurfaceData hitSurface = ReconstructRayTracingHitSurface(trace, path.DirectionWorld);
			if (!hitSurface.Valid)
			{
				return PathTracer::InvalidRadiance().rgb;
			}

			const RayTracingPathSurface surface = BuildHitRayTracingPathSurface(hitSurface, path.DirectionWorld);
			const uint surfaceVertexCount = path.SurfaceDepth + 1u;
			if (FinitePathDiagnosticSurfaceVertices == 0u && surfaceVertexCount > 4096u)
			{
				return PathTracer::InvalidRadiance().rgb;
			}

			PathTracer::AddRadiance(contribution, path.Throughput, EvaluateSurfaceEmission(lightCounts, previousEvent, surface));

			const PathBsdf::LobeMasses lobeMasses = PathBsdf::BuildEqualLobeMasses(surface);

			PathTracer::AddRadiance(contribution,
			                        path.Throughput,
			                        SampleDirectLighting(sceneTlas,
			                                             skyTexture,
			                                             skySampler,
			                                             surface,
			                                             lobeMasses,
			                                             lightCounts,
			                                             sampleIdentity,
			                                             path.SurfaceDepth));

			if ((FinitePathDiagnosticSurfaceVertices != 0u && surfaceVertexCount == FinitePathDiagnosticSurfaceVertices)
			    || lobeMasses.Count == 0u)
			{
				break;
			}

			const RayTracingPathSample::DirectionSample bsdf = SamplePathDirection(surface, lobeMasses, sampleIdentity, path.SurfaceDepth);
			if (!bsdf.HasSupport)
			{
				break;
			}

			previousEvent.PositionWorld = surface.PositionWorld;
			previousEvent.BsdfPdfW = bsdf.PdfW;
			previousEvent.Delta = bsdf.Delta;

			PathTracer::ApplyDirectionSample(path, bsdf);

			if (path.SurfaceDepth >= 3u && !SurvivesRussianRoulette(path.Throughput, sampleIdentity, surfaceVertexCount))
			{
				break;
			}

			traversal =
			    RayEndpoints::Continuation(surface.PositionWorld, surface.GeometricNormalWorld, surface.PositionError, path.DirectionWorld);

			path.OriginWorld = traversal.Origin;
			path.DirectionWorld = traversal.Direction;
		}

		return contribution;
	}
}

#endif
