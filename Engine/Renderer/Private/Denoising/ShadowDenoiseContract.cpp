#include "../PCH.h"

#include "Denoising/ShadowDenoiseContract.h"

namespace ShadowDenoiseContract
{
	ShadowDenoiseContract BuildContract(const BuildRequest& request) noexcept
	{
		ShadowDenoiseContract contract{
		    .Stage = ShadowDenoiseStage::Off,
		    .UsesDenoiser = false,
		    .RaysPerPixel = request.RaysPerPixel,
		    .HasSceneTlas = request.HasSceneTlas,
		    .Inputs = request.Inputs,
		    .Textures = request.Textures,
		    .PreviousDenoisedVisibility = request.PreviousDenoisedVisibility,
		    .CurrentDenoisedVisibility = request.CurrentDenoisedVisibility};

		if (!request.HasSceneTlas || !request.Inputs.HasRawVisibility)
		{
			return contract;
		}

		contract.Stage = ShadowDenoiseStage::RawVisibility;
		const bool canRunSigma = request.RequestDenoiser && request.ProviderAvailable && request.InlineRayQueryAvailable &&
		                         request.Inputs.HasDepth && request.Inputs.HasNormals &&
		                         request.Inputs.HasMotionVectors && request.Inputs.HasJitter &&
		                         request.Inputs.HasHistory;
		if (canRunSigma)
		{
			contract.Stage = ShadowDenoiseStage::DenoisedVisibility;
			contract.UsesDenoiser = true;
		}

		return contract;
	}
}  // namespace ShadowDenoiseContract
