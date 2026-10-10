#include "../../PCH.h"
#include "Passes/Lighting/LightingComposite.h"

#include "Passes/Lighting/LightingCompositeShader.h"
#include "Passes/Lighting/LightingTargetClear.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include <array>

void AddLightingCompositePasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	const std::array sceneColorTarget{resources.Transient.Scene.SceneColor};
	AddLightingTargetClearPass(builder, "SceneColor.Clear", sceneExtent, sceneColorTarget);

	const auto& lighting = resources.Transient.Lighting;
	auto& parameters = builder.AllocParameters<LightingCompositeCS>();
	parameters->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferDeviceZ = builder.CreateSRV(resources.Transient.GBuffer.DeviceZ);
	parameters->DirectDiffuse = builder.CreateSRV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateSRV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateSRV(lighting.DirectSubsurface);
	parameters->IndirectDiffuse = builder.CreateSRV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateSRV(lighting.IndirectSpecular);
	parameters->GBufferEmissive = builder.CreateSRV(resources.Transient.GBuffer.Emissive);
	builder.Dispatch<LightingCompositeCS>(parameters, ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
