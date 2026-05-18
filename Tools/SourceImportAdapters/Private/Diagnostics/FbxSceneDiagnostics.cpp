#include "PCH.h"

#include "Diagnostics/FbxSceneDiagnostics.h"

#include <assimp/scene.h>

SourceSceneFeatureDiagnostics FbxSceneDiagnostics::CaptureFeatures(const aiScene& scene) noexcept
{
	SourceSceneFeatureDiagnostics diagnostics;
	diagnostics.animationCount = scene.mNumAnimations;
	diagnostics.embeddedTextureCount = scene.mNumTextures;
	diagnostics.cameraNodeCount = scene.mNumCameras;
	diagnostics.lightNodeCount = scene.mNumLights;
	return diagnostics;
}