#include "PCH.h"

#include "SceneAssetCameraTranslator.h"

#include "Scene/Transform.h"

#include <DirectXMath.h>

namespace Assets
{
	SceneAssetPayload::Camera BuildSceneAssetCamera(const CookedSceneCameraRecord& cameraRecord)
	{
		const Transform cameraTransform(DirectX::XMLoadFloat4x4(&cameraRecord.worldTransform));
		const DirectX::XMFLOAT3 rotationEuler = cameraTransform.GetRotationEuler();

		SceneAssetPayload::Camera camera;
		camera.name = cameraRecord.name;
		camera.desc.position = cameraTransform.GetTranslation();
		camera.desc.pitchRadians = rotationEuler.x;
		camera.desc.yawRadians = rotationEuler.y;
		camera.desc.fovYDegrees = cameraRecord.fovYRadians * 180.0f / DirectX::XM_PI;
		camera.desc.nearZ = cameraRecord.nearZ;
		camera.desc.farZ = cameraRecord.farZ;
		camera.desc.projectionKind = CameraProjectionKind::Perspective;
		return camera;
	}
}  // namespace Assets
