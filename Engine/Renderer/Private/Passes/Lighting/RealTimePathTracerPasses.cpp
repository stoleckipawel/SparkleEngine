#include "../../PCH.h"
#include "Passes/Lighting/RealTimePathTracerPasses.h"

#include "Frame/RenderFrame.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/GBuffer/GBufferPasses.h"
#include "Passes/Lighting/LightingComposite.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"
#include "Passes/Lighting/Restir/RestirLightingPasses.h"
#include "Passes/Lighting/Sky/Sky.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Passes/Lighting/Shadows/DirectShadowControls.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"

std::uint64_t GetRealTimePathTracerGraphRebuildKey() noexcept
{
	constexpr std::uint64_t directLightingPassesBit = 1u << 0u;
	constexpr std::uint64_t directShadowPassBit = 1u << 1u;
	constexpr std::uint64_t indirectLightingPassesBit = 1u << 2u;
	constexpr std::uint64_t skyBackgroundPassesBit = 1u << 3u;

	return (IsDirectLightingAdmitted() ? directLightingPassesBit : 0u) | (IsDirectShadowsActive() ? directShadowPassBit : 0u)
	    | (IsIndirectLightingAdmitted() ? indirectLightingPassesBit : 0u) | (CVarSkyEnabled.Get() ? skyBackgroundPassesBit : 0u);
}

void AddRealTimePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RenderFrameGraphResources& resources)
{
	AddGBufferPasses(builder, frame, settings.RenderExtent, gpuMeshCache, rayTracingScene, resources);

	AddRestirLightingPasses(builder, frame, settings.RenderExtent, rayTracingScene, resources);
	AddLightingCompositePasses(builder, settings.RenderExtent, resources);
	AddSkyPass(builder, frame, settings.RenderExtent, resources);

	PublishRealTimePathTracerProducts(resources);
}
