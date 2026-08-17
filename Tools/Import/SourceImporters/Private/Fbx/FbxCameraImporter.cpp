#include "PCH.h"

#include "Fbx/FbxCameraImporter.h"

#include "Fbx/FbxNodeTransformConverter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <DirectXMath.h>

#include <cmath>
#include <format>
#include <limits>

float FbxCameraImporter::ResolveFovYRadians(const aiCamera& camera) noexcept
{
	return 2.0f * std::atan(std::tan(camera.mHorizontalFOV) / camera.mAspect);
}

void FbxCameraImporter::ImportCameras(const aiScene& scene, float sourceMetersPerUnit, SourceImportOutput& output)
{
	output.scene.cameras.reserve(scene.mNumCameras);
	for (unsigned int cameraIndex = 0; cameraIndex < scene.mNumCameras; ++cameraIndex)
	{
		const aiCamera* sourceCamera = scene.mCameras[cameraIndex];
		const aiNode* node = sourceCamera != nullptr ? FbxNodeTransformConverter::FindNode(scene, sourceCamera->mName) : nullptr;
		if (sourceCamera == nullptr || sourceCamera->mName.length == 0 || node == nullptr || sourceCamera->mOrthographicWidth != 0.0f)
		{
			throw Diagnostics::Error(std::format("FBX camera {} has incomplete source data or an unsupported projection.", cameraIndex));
		}

		ImportedCamera camera;
		camera.name = sourceCamera->mName.C_Str();
		camera.sourceNodeIndex = FbxNodeTransformConverter::FindNodeIndex(scene, *node);
		camera.projectionKind = ImportedCameraProjectionKind::Perspective;
		camera.fovYRadians = ResolveFovYRadians(*sourceCamera);
		camera.nearZ = sourceCamera->mClipPlaneNear * sourceMetersPerUnit;
		camera.farZ = sourceCamera->mClipPlaneFar * sourceMetersPerUnit;
		if (camera.sourceNodeIndex == (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error(std::format("FBX camera '{}' cannot be mapped to an engine camera.", camera.name));
		}

		// Assimp's left-handed postprocess leaves camera position untouched and
		// rewrites LookAt as 2 * Position - LookAt. Recover the authored direction,
		// then reflect camera-local spatial values exactly once to match meshes/nodes.
		const aiVector3D authoredDirection = sourceCamera->mPosition * 2.0f - sourceCamera->mLookAt;
		const aiVector3D position =
		    aiVector3D(sourceCamera->mPosition.x, sourceCamera->mPosition.y, -sourceCamera->mPosition.z) * sourceMetersPerUnit;
		const aiVector3D direction(authoredDirection.x, authoredDirection.y, -authoredDirection.z);
		const aiVector3D up(sourceCamera->mUp.x, sourceCamera->mUp.y, -sourceCamera->mUp.z);
		camera.worldTransform = FbxNodeTransformConverter::BuildNodeAttachedOrientation(*node, position, direction, up);

		output.scene.cameras.push_back(std::move(camera));
	}
}
