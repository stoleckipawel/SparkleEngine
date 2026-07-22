#include "PCH.h"
#include "World/Extraction/RenderInputExtractor.h"

#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"

namespace ECS
{
	RenderInputFrame RenderInputExtractor::Extract(
	    GameWorldState& state,
	    GameWorldResourceStores& resources,
	    const WorldReadView& readView,
	    std::uint64_t sceneGeneration,
	    RenderFrameMetadata metadata)
	{
		RenderInputFrame frame;
		BeginFrame(frame, sceneGeneration, metadata);

		const auto meshes = state.GetExtractedMeshes();
		m_objectExtractor.Extract(meshes, m_identities, frame.WorldDelta);
		m_resourcePublisher.Publish(
		    state.GetExtractedMeshGroups(), state.ReadSkyEnvironment(), resources, frame.WorldDelta);
		m_dynamicExtractor.Extract(
		    state, resources, readView, meshes, m_objectExtractor, m_identities, frame.Dynamic);
		return frame;
	}

	void RenderInputExtractor::BeginFrame(
	    RenderInputFrame& frame,
	    std::uint64_t sceneGeneration,
	    RenderFrameMetadata metadata)
	{
		frame.Dynamic.Metadata = metadata;
		frame.Dynamic.Metadata.FrameGeneration = sceneGeneration;
		frame.WorldDelta.SceneGeneration = sceneGeneration;
		frame.WorldDelta.SequenceNumber = ++m_sequence;
		frame.WorldDelta.ResetScene = m_sceneGeneration != sceneGeneration;
		frame.Dynamic.Metadata.CameraCut |= frame.WorldDelta.ResetScene;
		frame.Dynamic.Metadata.ResetHistory |= frame.WorldDelta.ResetScene || frame.Dynamic.Metadata.CameraCut ||
		                                      frame.Dynamic.Metadata.CameraTeleported;
		if (!frame.WorldDelta.ResetScene) return;

		m_identities.BeginScene();
		m_objectExtractor.BeginScene();
		m_resourcePublisher.BeginScene();
		m_sceneGeneration = sceneGeneration;
	}
}
