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

void FbxCameraImporter::ImportCameras(const aiScene& scene, SourceImportOutput& output)
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
		camera.nearZ = sourceCamera->mClipPlaneNear;
		camera.farZ = sourceCamera->mClipPlaneFar;
		if (camera.sourceNodeIndex == (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error(std::format("FBX camera '{}' cannot be mapped to an engine camera.", camera.name));
		}
		camera.worldTransform = FbxNodeTransformConverter::BuildNodeAttachedOrientation(
		    *node,
		    sourceCamera->mPosition,
		    sourceCamera->mLookAt,
		    sourceCamera->mUp);

		output.scene.cameras.push_back(std::move(camera));
	}
}
