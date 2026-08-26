#include "PCH.h"

#include "Passes/RayTracing/RayTracingGBufferShaders.h"

IMPLEMENT_GLOBAL_SHADER(
	RayTracingGBufferInlineCS,
	"/Engine/Passes/RayTracing/RayTracingGBufferInline.hlsl",
	"RayTracingGBufferInline",
	Compute);
IMPLEMENT_GLOBAL_SHADER(
	RayTracingGBufferRGS,
	"/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl",
	"RayTracingGBufferRayGeneration",
	RayGeneration);
IMPLEMENT_GLOBAL_SHADER(
	RayTracingGBufferMiss,
	"/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl",
	"RayTracingGBufferMiss",
	Miss);
IMPLEMENT_GLOBAL_SHADER(
	RayTracingGBufferClosestHit,
	"/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl",
	"RayTracingGBufferClosestHit",
	ClosestHit);
IMPLEMENT_GLOBAL_SHADER(
	RayTracingGBufferAnyHit,
	"/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl",
	"RayTracingGBufferAnyHit",
	AnyHit);
