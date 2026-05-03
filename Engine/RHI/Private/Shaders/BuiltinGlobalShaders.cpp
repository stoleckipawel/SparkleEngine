#include "PCH.h"

void RegisterHelloTriangleShaders() noexcept;
void RegisterGBufferShaders() noexcept;
void RegisterDeferredLightingShaders() noexcept;
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
	RegisterDeferredLightingShaders();
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