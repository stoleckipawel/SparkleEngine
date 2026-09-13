#ifndef SPARKLE_RAY_TRACING_TRACE_RESULT_HLSLI
#define SPARKLE_RAY_TRACING_TRACE_RESULT_HLSLI

struct RayTracingTraceResult
{
	bool Hit;
	bool FrontFace;
	float RayT;
	uint InstanceId;
	uint PrimitiveIndex;
	float2 Barycentrics;
};

#endif
