#include "PCH.h"

#include "GameWorldSystemGraphBuilder.h"
#include "GameWorldSystemContract.h"

namespace ECS
{
	CompiledGameSystemGraph BuildGameWorldSystemGraph()
	{
		GameSystemGraph graph;
		GameSystemDesc camera{GameWorldSystemIds::CameraMovement, "Game.CameraMovement", GameSystemPhase::Simulation};
		camera.DeclareQuery<CameraMovementQuery>();
		camera.Resources = {{GameSystemResourceDomain::UpdateInputs, GameSystemAccessMode::Read},
		                    {GameSystemResourceDomain::CameraInputIntent, GameSystemAccessMode::Read},
		                    {GameSystemResourceDomain::SystemChangeScratch, GameSystemAccessMode::Write}};
		camera.Execution = ParallelRanges(GameWorldSystemGrain::Camera);
		graph.Add(std::move(camera));

		GameSystemDesc motion{GameWorldSystemIds::OscillatingMeshMotion, "Game.OscillatingMeshMotion", GameSystemPhase::Simulation};
		motion.DeclareQuery<OscillatingMotionQuery>();
		motion.Resources = {{GameSystemResourceDomain::UpdateInputs, GameSystemAccessMode::Read},
		                    {GameSystemResourceDomain::MotionClock, GameSystemAccessMode::Read},
		                    {GameSystemResourceDomain::SystemChangeScratch, GameSystemAccessMode::Write}};
		motion.Prerequisites = {GameWorldSystemIds::CameraMovement};
		motion.Execution = ParallelRanges(GameWorldSystemGrain::Motion);
		graph.Add(std::move(motion));

		GameSystemDesc playback{GameWorldSystemIds::PlaybackAdvance, "Game.AnimationPlaybackAdvance", GameSystemPhase::Animation};
		playback.DeclareQuery<PlaybackAdvanceQuery>();
		playback.Resources = {{GameSystemResourceDomain::UpdateInputs, GameSystemAccessMode::Read},
		                      {GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read},
		                      {GameSystemResourceDomain::SystemChangeScratch, GameSystemAccessMode::Write}};
		playback.Execution = ParallelRanges(GameWorldSystemGrain::Animation);
		graph.Add(std::move(playback));

		GameSystemDesc pose{GameWorldSystemIds::PoseEvaluation, "Game.AnimationPoseEvaluation", GameSystemPhase::Animation};
		pose.DeclareQuery<PoseEvaluationQuery>();
		pose.Resources = {{GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read},
		                  {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		                  {GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Write}};
		pose.Prerequisites = {GameWorldSystemIds::PlaybackAdvance};
		pose.Execution = ParallelRanges(GameWorldSystemGrain::Pose);
		graph.Add(std::move(pose));

		GameSystemDesc morph{GameWorldSystemIds::MorphWeightEvaluation, "Game.MorphWeightEvaluation", GameSystemPhase::Animation};
		morph.DeclareQuery<MorphEvaluationQuery>();
		morph.Resources = {{GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read},
		                   {GameSystemResourceDomain::MorphScratch, GameSystemAccessMode::Write}};
		morph.Prerequisites = {GameWorldSystemIds::PlaybackAdvance};
		morph.Execution = ParallelRanges(GameWorldSystemGrain::Animation);
		graph.Add(std::move(morph));

		GameSystemDesc skinning{GameWorldSystemIds::SkinningMatrixEvaluation, "Game.SkinningMatrixEvaluation", GameSystemPhase::Deformation};
		skinning.Resources = {{GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Read},
		                      {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		                      {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Write}};
		skinning.Prerequisites = {GameWorldSystemIds::PoseEvaluation};
		skinning.Execution = ParallelRanges(GameWorldSystemGrain::Pose);
		graph.Add(std::move(skinning));

		GameSystemDesc morphCommit{GameWorldSystemIds::MorphOutputCommit, "Game.MorphOutputCommit", GameSystemPhase::Deformation};
		morphCommit.Resources = {{GameSystemResourceDomain::MorphScratch, GameSystemAccessMode::Read},
		                         {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Write}};
		morphCommit.Prerequisites = {GameWorldSystemIds::MorphWeightEvaluation};
		morphCommit.Execution = ParallelRanges(GameWorldSystemGrain::SingleItem);
		graph.Add(std::move(morphCommit));

		GameSystemDesc outputCommit{GameWorldSystemIds::SystemOutputCommit, "Game.SystemOutputCommit", GameSystemPhase::Deformation};
		outputCommit.Resources = {{GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Read},
		                         {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Read},
		                         {GameSystemResourceDomain::SystemChangeScratch, GameSystemAccessMode::Read},
		                         {GameSystemResourceDomain::MotionClock, GameSystemAccessMode::Write},
		                         {GameSystemResourceDomain::DirtyTransforms, GameSystemAccessMode::Write},
		                         {GameSystemResourceDomain::WorldChanges, GameSystemAccessMode::Write}};
		outputCommit.Prerequisites = {GameWorldSystemIds::SkinningMatrixEvaluation, GameWorldSystemIds::MorphOutputCommit};
		outputCommit.Execution = ParallelRanges(GameWorldSystemGrain::SingleItem);
		graph.Add(std::move(outputCommit));

		GameSystemDesc transform{GameWorldSystemIds::TransformEvaluation, "Game.TransformEvaluation", GameSystemPhase::Transform};
		transform.DeclareQuery<TransformEvaluationQuery>();
		transform.Resources = {{GameSystemResourceDomain::DirtyTransforms, GameSystemAccessMode::Read},
		                       {GameSystemResourceDomain::TransformScratch, GameSystemAccessMode::Write}};
		transform.Execution = ParallelRanges(GameWorldSystemGrain::Transform);
		graph.Add(std::move(transform));

		GameSystemDesc cameraDerived{GameWorldSystemIds::CameraDerivedState, "Game.CameraDerivedState", GameSystemPhase::Transform};
		cameraDerived.DeclareQuery<CameraDerivedStateQuery>();
		cameraDerived.Resources = {{GameSystemResourceDomain::DirtyTransforms, GameSystemAccessMode::Read},
		                           {GameSystemResourceDomain::CameraDerivedScratch, GameSystemAccessMode::Write}};
		cameraDerived.Prerequisites = {GameWorldSystemIds::TransformEvaluation};
		cameraDerived.Execution = ParallelRanges(GameWorldSystemGrain::Transform);
		graph.Add(std::move(cameraDerived));

		GameSystemDesc extraction{GameWorldSystemIds::MeshExtraction, "Game.MeshExtraction", GameSystemPhase::Extraction};
		extraction.DeclareQuery<MeshExtractionQuery>();
		extraction.Resources = {{GameSystemResourceDomain::MeshResources, GameSystemAccessMode::Read},
		                        {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Read},
		                        {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Read},
		                        {GameSystemResourceDomain::ExtractionScratch, GameSystemAccessMode::Write}};
		extraction.Execution = ParallelRanges(GameWorldSystemGrain::Extraction);
		graph.Add(std::move(extraction));

		GameSystemDesc extractionCommit{GameWorldSystemIds::ExtractionCommit, "Game.ExtractionCommit", GameSystemPhase::Extraction};
		extractionCommit.Resources = {{GameSystemResourceDomain::ExtractionScratch, GameSystemAccessMode::Read},
		                              {GameSystemResourceDomain::ExtractionOutput, GameSystemAccessMode::Write},
		                              {GameSystemResourceDomain::TransformScratch, GameSystemAccessMode::Read},
		                              {GameSystemResourceDomain::CameraDerivedScratch, GameSystemAccessMode::Read},
		                              {GameSystemResourceDomain::WorldChanges, GameSystemAccessMode::Write},
		                              {GameSystemResourceDomain::WorldPublication, GameSystemAccessMode::Write}};
		extractionCommit.Prerequisites = {GameWorldSystemIds::MeshExtraction};
		extractionCommit.Execution = ParallelRanges(GameWorldSystemGrain::SingleItem);
		graph.Add(std::move(extractionCommit));
		return graph.Compile();
	}
}
