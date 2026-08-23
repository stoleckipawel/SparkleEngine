#include "/Engine/Passes/Lighting/Shadows/DirectShadowSignalCommon.hlsli"

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	EvaluateDirectShadowSignal(dispatchThreadId);
}
