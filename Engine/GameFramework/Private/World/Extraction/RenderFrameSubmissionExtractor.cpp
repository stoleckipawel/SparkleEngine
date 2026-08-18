#include "PCH.h"
#include "World/Extraction/RenderFrameSubmissionExtractor.h"

#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"

namespace ECS
{
	RenderFrameSubmission RenderFrameSubmissionExtractor::Extract(
	    GameWorldState& state,
	    GameWorldResourceStores& resources,
	    const WorldReadView& readView,
	    std::uint64_t sceneGeneration,
	    std::uint64_t frameId)
	{
		RenderFrameSubmission submission;
		BeginFrame(submission, sceneGeneration, frameId);
		submission.View.Camera = BuildViewCamera(readView);

		const auto meshes = state.GetExtractedMeshes();
		m_objectExtractor.Extract(meshes, m_identities, submission.Scene.Structural);
		m_resourcePublisher.Publish(state.GetExtractedMeshGroups(), state.ReadSkyEnvironment(), resources, submission.Scene.Structural);
		m_dynamicExtractor.Extract(state, resources, readView, meshes, m_objectExtractor, m_identities, submission.Scene.Dynamic);
		return submission;
	}

	void RenderFrameSubmissionExtractor::BeginFrame(RenderFrameSubmission& submission, std::uint64_t sceneGeneration, std::uint64_t frameId)
	{
		submission.FrameId = frameId;
		submission.Scene.Structural.SceneGeneration = sceneGeneration;
		submission.Scene.Structural.SequenceNumber = ++m_sequence;
		submission.Scene.Structural.ResetScene = m_sceneGeneration != sceneGeneration;
		submission.View.CameraCut = submission.Scene.Structural.ResetScene;
		if (!submission.Scene.Structural.ResetScene)
			return;

		m_identities.BeginScene();
		m_objectExtractor.BeginScene();
		m_resourcePublisher.BeginScene();
		m_dynamicExtractor.BeginScene();
		m_sceneGeneration = sceneGeneration;
	}

	RenderViewCameraData RenderFrameSubmissionExtractor::BuildViewCamera(const WorldReadView& readView) noexcept
	{
		for (const WorldCameraReadData& camera : readView.GetCameras())
		{
			if (!camera.Active)
				continue;
			return {
			    .Position = camera.LocalTransform.GetTranslation(),
			    .Direction = camera.Direction,
			    .FovYDegrees = camera.Description.fovYDegrees,
			    .AspectRatio = camera.AspectRatio,
			    .NearZ = camera.Description.nearZ,
			    .FarZ = camera.Description.farZ,
			    .ProjectionKind = camera.Description.projectionKind};
		}
		return {};
	}
}
