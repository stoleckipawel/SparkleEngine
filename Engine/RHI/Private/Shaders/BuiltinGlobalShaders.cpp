#include "PCH.h"

void RegisterHelloTriangleShaders() noexcept;
void RegisterForwardOpaqueShaders() noexcept;
void RegisterShadowOpaqueShaders() noexcept;
void RegisterComputeClearShaders() noexcept;

void RegisterBuiltinGlobalShaders() noexcept
{
	RegisterHelloTriangleShaders();
	RegisterForwardOpaqueShaders();
	RegisterShadowOpaqueShaders();
	RegisterComputeClearShaders();
}