#include "../../PCH.h"
#include "Frame/Lighting/ShadowDenoise.h"

#include "Denoising/ShadowDenoiseContract.h"
#include "FrameGraph/Resources/FrameGraphDenoiserRegistration.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"

FrameGraphTextureHandle CreateShadowVisibilityResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources)
{
	const RayTracedShadowSettings shadowSettings = BuildRayTracedShadowSettingsFromCVars();
	const bool requestSigma = shadowSettings.DenoiserMode == RayTracedShadowDenoiserMode::NrdSigma;
	constexpr bool nrdSigmaProviderAvailable = false;

	resources.Transient.ShadowDenoiser = FrameGraphDenoiserRegistration::RegisterShadowVisibilityResources(
	    builder,
	    sceneExtent,
	    requestSigma && nrdSigmaProviderAvailable);

	const ShadowDenoiseContract::ShadowDenoiseContract contract = ShadowDenoiseContract::BuildContract(
	    ShadowDenoiseContract::BuildRequest{
	        .RequestDenoiser = requestSigma,
	        .ProviderAvailable = nrdSigmaProviderAvailable,
	        .InlineRayQueryAvailable = true,
	        .HasSceneTlas = resources.Persistent.SceneTlas.IsValid(),
	        .RaysPerPixel = RayTracedShadowSettings::RaysPerPixel,
	        .Inputs =
	            ShadowDenoiseContract::ShadowDenoiseInputState{
	                .HasRawVisibility = resources.Transient.ShadowDenoiser.PackedSignal.IsValid(),
	                .HasDepth = resources.Transient.GBuffer.DeviceZ.IsValid(),
	                .HasNormals = resources.Transient.GBuffer.Normal.IsValid(),
	                .HasMotionVectors = resources.Transient.GBuffer.MotionVector.IsValid(),
	                .HasJitter = true,
	                .HasHistory = resources.History.HasShadowDenoiseHistory()},
	        .Textures = resources.Transient.ShadowDenoiser,
	        .PreviousDenoisedVisibility = resources.History.PreviousDenoisedShadowVisibility,
	        .CurrentDenoisedVisibility = resources.History.CurrentDenoisedShadowVisibility});

	return contract.Textures.PackedSignal;
}
