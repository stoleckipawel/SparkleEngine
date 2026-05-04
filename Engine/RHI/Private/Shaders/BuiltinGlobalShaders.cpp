#include "PCH.h"

void RegisterHelloTriangleShaders() noexcept;
void RegisterGBufferShaders() noexcept;
void RegisterDirectLightingShaders() noexcept;
void RegisterIndirectLightingShaders() noexcept;
void RegisterLightingCompositeShaders() noexcept;
void RegisterSkyShaders() noexcept;
void RegisterVisualizeBuffersShaders() noexcept;
void RegisterComputeClearShaders() noexcept;
void RegisterHelloInlineRayQueryCSShader() noexcept;
void RegisterHelloRayGenShader() noexcept;
void RegisterHelloMissShader() noexcept;
void RegisterHelloClosestHitShader() noexcept;
void RegisterHelloAnyHitShader() noexcept;
void RegisterHelloIntersectionShader() noexcept;
void RegisterHelloCallableShader() noexcept;
void RegisterHelloPrimaryHitGroup() noexcept;

void RegisterBuiltinGlobalShaders() noexcept
{
	RegisterHelloTriangleShaders();
	RegisterGBufferShaders();
	RegisterDirectLightingShaders();
	RegisterIndirectLightingShaders();
	RegisterLightingCompositeShaders();
	RegisterSkyShaders();
	RegisterVisualizeBuffersShaders();
	RegisterComputeClearShaders();
	RegisterHelloInlineRayQueryCSShader();
	RegisterHelloRayGenShader();
	RegisterHelloMissShader();
	RegisterHelloClosestHitShader();
	RegisterHelloAnyHitShader();
	RegisterHelloIntersectionShader();
	RegisterHelloCallableShader();
	RegisterHelloPrimaryHitGroup();
}