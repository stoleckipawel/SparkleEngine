#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Debug/RendererCVars.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Lighting/LightingSceneState.h"
#include "Frame/Lighting/ReferenceLightingState.h"
#include "Host/RendererSystemRoot.h"
#include "Providers/RendererImageProviderStack.h"
#include "Textures/TextureManager.h"

void FramePipeline::UpdateLightingHistoryState(const FrameContext& frame) noexcept
{
	const Texture* environmentTexture = m_systems->GetTextureManager().ResolveEnvironmentMapTexture();
	switch (GetLightingMode())
	{
		case LightingMode::RestirPathTraced:
		{
			const std::uint64_t stateKey = BuildLightingSceneStateKey(frame, environmentTexture);
			if (m_restirLightingSceneStateKey != 0u && stateKey != m_restirLightingSceneStateKey)
			{
				ResetRestirLightingHistory();
				m_systems->GetImageProviders().ResetHistory("ReSTIR lighting scene changed");
			}
			m_restirLightingSceneStateKey = stateKey;
			break;
		}
		case LightingMode::ReferencePathTraced:
		{
			const std::uint64_t stateKey = BuildReferenceLightingStateKey(frame, environmentTexture);
			if (m_referenceLightingStateKey != 0u && stateKey != m_referenceLightingStateKey)
			{
				ResetReferenceLightingHistory();
			}
			m_referenceLightingStateKey = stateKey;
			break;
		}
	}
}
