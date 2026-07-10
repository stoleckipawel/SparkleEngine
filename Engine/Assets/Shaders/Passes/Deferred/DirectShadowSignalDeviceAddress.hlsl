#define SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS 1
#include "Passes/Deferred/DirectShadowSignalCommon.hlsli"

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	EvaluateDirectShadowSignal(dispatchThreadId);
}
