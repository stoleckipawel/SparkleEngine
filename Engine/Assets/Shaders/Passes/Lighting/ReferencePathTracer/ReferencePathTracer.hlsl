#include "/Engine/Resources/ViewCameraRay.hlsli"

#include "/Engine/RayTracing/PathBsdf.hlsli"
#include "/Engine/RayTracing/PathTracer.hlsli"
#include "/Engine/RayTracing/PathVisibility.hlsli"
#include "/Engine/RayTracing/RayEndpoints.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialHit.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialTraceQuery.hlsli"
#include "/Engine/RayTracing/RayTracingHitUniformData.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerLightSampling.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerSampler.hlsli"

RWTexture2D<float4> SceneColor;
RaytracingAccelerationStructure SceneTlas;
Texture2D SkyTexture;
SamplerState SamplerLinearWrapClamp;

cbuffer ReferencePathTracerConstants
{
	uint SessionSeed;
	uint ReplicateId;
	uint SampleOrdinal;
	uint FinitePathDiagnosticSurfaceVertices;
};

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width;
	uint height;
	SceneColor.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	const ReferencePathTracerLightSampling::Counts lightCounts = ReferencePathTracerLightSampling::GetCounts();
	const float2 filmSample = float2(
	    CommonRandom::OpenUnitInterval(
	        ReferencePathTracerSampler::Word(pixelCoord, SampleOrdinal, ReferencePathTracerSampler::FilmX, SessionSeed, ReplicateId)),
	    CommonRandom::OpenUnitInterval(
	        ReferencePathTracerSampler::Word(pixelCoord, SampleOrdinal, ReferencePathTracerSampler::FilmY, SessionSeed, ReplicateId)));
	const ViewCameraRay cameraRay = BuildPerspectiveViewCameraRay(pixelCoord, uint2(width, height), filmSample);
	RayEndpoints::Ray traversal =
	    RayEndpoints::Primary(cameraRay.OriginWorld, cameraRay.DirectionWorld, cameraRay.TMin, cameraRay.TMax);

	PathTracer::PathState path;
	path.OriginWorld = traversal.Origin;
	path.DirectionWorld = traversal.Direction;
	path.Throughput = 1.0f.xxx;
	path.SurfaceDepth = 0u;
	float3 contribution = 0.0f.xxx;
	float3 previousPositionWorld = 0.0f.xxx;
	float previousBsdfPdfW = 0.0f;
	bool previousEventDelta = true;

	[loop] for (;;)
	{
		const RayTracingTraceResult trace = TraceRayQueryWithAlphaTest(SceneTlas,
		                                                                path.OriginWorld,
		                                                                path.DirectionWorld,
		                                                                traversal.TMin,
		                                                                traversal.TMax,
		                                                                RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
		                                                                    | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		                                                                0xFFu);
		if (!trace.Hit)
		{
			const float lightPdfW = ReferencePathTracerLightSampling::EnvironmentPdfW(lightCounts);
			const float misWeight = previousEventDelta ? 1.0f : PathTracer::PowerHeuristic(previousBsdfPdfW, lightPdfW);
			const float3 environmentRadiance =
			    SampleSkyRadiance(SkyTexture, SamplerLinearWrapClamp, path.DirectionWorld) * misWeight;
			PathTracer::AddRadiance(contribution, path.Throughput, environmentRadiance);
			break;
		}

		const RayTracingHitSurfaceData hitSurface = ReconstructRayTracingHitSurface(trace, path.DirectionWorld);
		if (!hitSurface.Valid)
		{
			SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
			return;
		}
		const RayTracingPathSurface surface = BuildHitRayTracingPathSurface(hitSurface, path.DirectionWorld);
		const uint surfaceVertexCount = path.SurfaceDepth + 1u;
		if (FinitePathDiagnosticSurfaceVertices == 0u && surfaceVertexCount > 4096u)
		{
			SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
			return;
		}

		float emissionMisWeight = 1.0f;
		if (!previousEventDelta && any(surface.EmissiveColor > 0.0f))
		{
			const float lightPdfW =
			    ReferencePathTracerLightSampling::EmissiveTrianglePdfW(lightCounts, previousPositionWorld, surface);
			emissionMisWeight = PathTracer::PowerHeuristic(previousBsdfPdfW, lightPdfW);
		}
		PathTracer::AddRadiance(contribution, path.Throughput, surface.EmissiveColor * emissionMisWeight);

		const PathBsdf::LobeMasses lobeMasses = PathBsdf::BuildEqualLobeMasses(surface);
		if (lightCounts.Total != 0u
		    && (lobeMasses.Diffuse > 0.0f || (lobeMasses.Specular > 0.0f && !lobeMasses.SpecularDelta)))
		{
			const uint lightChoiceDimension = ReferencePathTracerSampler::SurfaceDimension(
			    path.SurfaceDepth, ReferencePathTracerSampler::LightChoiceOffset);
			const uint lightShapeXDimension = ReferencePathTracerSampler::SurfaceDimension(
			    path.SurfaceDepth, ReferencePathTracerSampler::LightShapeXOffset);
			const uint lightShapeYDimension = ReferencePathTracerSampler::SurfaceDimension(
			    path.SurfaceDepth, ReferencePathTracerSampler::LightShapeYOffset);
			const ReferencePathTracerLightSampling::Selection selection = ReferencePathTracerLightSampling::Select(
			    lightCounts,
			    ReferencePathTracerSampler::Word(pixelCoord, SampleOrdinal, lightChoiceDimension, SessionSeed, ReplicateId));
			const float2 lightShapeSample = float2(
			    CommonRandom::OpenUnitInterval(ReferencePathTracerSampler::Word(
			        pixelCoord, SampleOrdinal, lightShapeXDimension, SessionSeed, ReplicateId)),
			    CommonRandom::OpenUnitInterval(ReferencePathTracerSampler::Word(
			        pixelCoord, SampleOrdinal, lightShapeYDimension, SessionSeed, ReplicateId)));
			const LightSampling::DirectLightSample light = ReferencePathTracerLightSampling::Sample(
			    selection, surface.PositionWorld, lightShapeSample, SkyTexture, SamplerLinearWrapClamp);
			const PathBsdf::Evaluation bsdf = PathBsdf::EvaluateContinuous(surface, light.DirectionWorld, lobeMasses);
			if (bsdf.HasSupport)
			{
				if (PathVisibility::IsUnoccluded(SceneTlas, surface, light))
				{
					const float lightProbability =
					    light.LightSelectionPdf * (light.Delta ? 1.0f : light.PdfW);
					const float misWeight =
					    light.Delta ? 1.0f : PathTracer::PowerHeuristic(lightProbability, bsdf.PdfW);
					const float3 directRadiance =
					    bsdf.F * light.IncidentRadiance * bsdf.Cosine * misWeight / lightProbability;
					PathTracer::AddRadiance(contribution, path.Throughput, directRadiance);
				}
			}
		}

		if (FinitePathDiagnosticSurfaceVertices != 0u
		    && surfaceVertexCount == FinitePathDiagnosticSurfaceVertices)
		{
			break;
		}
		if (lobeMasses.Count == 0u)
		{
			break;
		}

		const uint lobeChoiceDimension = ReferencePathTracerSampler::SurfaceDimension(
		    path.SurfaceDepth, ReferencePathTracerSampler::LobeChoiceOffset);
		const CommonRandom::CategoricalSample lobeChoice = CommonRandom::SampleCategorical(
		    ReferencePathTracerSampler::Word(pixelCoord, SampleOrdinal, lobeChoiceDimension, SessionSeed, ReplicateId),
		    lobeMasses.Count);
		uint selectedLobe = RayTracingPathSample::LobeSpecular;
		if (lobeMasses.Diffuse > 0.0f && lobeChoice.Index == 0u)
		{
			selectedLobe = RayTracingPathSample::LobeDiffuse;
		}
		const uint bsdfXDimension = ReferencePathTracerSampler::SurfaceDimension(
		    path.SurfaceDepth, ReferencePathTracerSampler::BsdfDirectionXOffset);
		const uint bsdfYDimension = ReferencePathTracerSampler::SurfaceDimension(
		    path.SurfaceDepth, ReferencePathTracerSampler::BsdfDirectionYOffset);
		const float2 bsdfSample = float2(
		    CommonRandom::OpenUnitInterval(ReferencePathTracerSampler::Word(
		        pixelCoord, SampleOrdinal, bsdfXDimension, SessionSeed, ReplicateId)),
		    CommonRandom::OpenUnitInterval(ReferencePathTracerSampler::Word(
		        pixelCoord, SampleOrdinal, bsdfYDimension, SessionSeed, ReplicateId)));
		const RayTracingPathSample::DirectionSample bsdf = PathBsdf::Sample(surface, lobeMasses, selectedLobe, bsdfSample);
		if (!bsdf.HasSupport)
		{
			break;
		}

		previousPositionWorld = surface.PositionWorld;
		previousBsdfPdfW = bsdf.PdfW;
		previousEventDelta = bsdf.Delta;
		PathTracer::ApplyDirectionSample(path, bsdf);

		if (path.SurfaceDepth >= 3u)
		{
			const float targetSurvival =
			    clamp(max(path.Throughput.r, max(path.Throughput.g, path.Throughput.b)), 0.05f, 0.95f);
			const float scaledSurvival = targetSurvival * 16777216.0f;
			const uint lowerThreshold = (uint)floor(scaledSurvival);
			const float thresholdFraction = scaledSurvival - (float)lowerThreshold;
			const uint roundedThreshold = lowerThreshold
			    + (thresholdFraction > 0.5f || (thresholdFraction == 0.5f && (lowerThreshold & 1u) != 0u) ? 1u : 0u);
			const uint threshold = min(max(roundedThreshold, 1u), 16777215u);
			const uint rouletteDimension = ReferencePathTracerSampler::SurfaceDimension(
			    surfaceVertexCount - 1u, ReferencePathTracerSampler::RouletteOffset);
			const uint rouletteValue = ReferencePathTracerSampler::Word(
			                                   pixelCoord, SampleOrdinal, rouletteDimension, SessionSeed, ReplicateId)
			                             >> 8u;
			if (rouletteValue >= threshold)
			{
				break;
			}
			PathTracer::ApplySurvivalCompensation(path.Throughput, (float)threshold * 0x1.0p-24f);
		}

		traversal = RayEndpoints::Continuation(
		    surface.PositionWorld, surface.GeometricNormalWorld, surface.PositionError, path.DirectionWorld);
		path.OriginWorld = traversal.Origin;
		path.DirectionWorld = traversal.Direction;
	}

	SceneColor[pixelCoord] = float4(contribution, 1.0f);
}
