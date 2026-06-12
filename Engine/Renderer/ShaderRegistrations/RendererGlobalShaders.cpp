#include "PCH.h"

#include "RendererShaderRegistration.h"

void RegisterGBufferShaders() noexcept;
void RegisterDirectLightingShaders() noexcept;
void RegisterIndirectLightingShaders() noexcept;
void RegisterLightingCompositeShaders() noexcept;
void RegisterSkyShaders() noexcept;
void RegisterVisualizeBuffersShaders() noexcept;
void RegisterComputeClearShaders() noexcept;

void RegisterRendererGlobalShaders() noexcept
{
	RegisterGBufferShaders();
	RegisterDirectLightingShaders();
	RegisterIndirectLightingShaders();
	RegisterLightingCompositeShaders();
	RegisterSkyShaders();
	RegisterVisualizeBuffersShaders();
	RegisterComputeClearShaders();
}
