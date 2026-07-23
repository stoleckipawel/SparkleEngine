#include "PCH.h"

#include "SceneAssetCameraTranslator.h"

#include "Scene/Transform.h"

#include <DirectXMath.h>

namespace Assets
{

		CameraProjectionKind ToCameraProjectionKind(CookedSceneCameraProjectionKind projectionKind) noexcept
		{
			switch (projectionKind)
			{
				case CookedSceneCameraProjectionKind::Perspective:
					return CameraProjectionKind::Perspective;
				case CookedSceneCameraProjectionKind::Orthographic:
					return CameraProjectionKind::Orthographic;
				case CookedSceneCameraProjectionKind::Unknown:
				default:
					return CameraProjectionKind::Unknown;
			}
		}

		std::string CookedCameraNameToString(const CookedSceneCameraRecord& cameraRecord, std::size_t cameraIndex)
		{
			std::size_t length = 0;
			while (length < kCookedSceneCameraNameCapacity && cameraRecord.name[length] != '\0')
			{
				++length;
			}

			if (length == 0)
			{
				return "Camera " + std::to_string(cameraIndex + 1);
			}

			return std::string(cameraRecord.name, length);
		}
	  // namespace

	SceneAssetPayload::Camera BuildSceneAssetCamera(const CookedSceneCameraRecord& cameraRecord, std::size_t cameraIndex)
	{
		const Transform cameraTransform(DirectX::XMLoadFloat4x4(&cameraRecord.worldTransform));
		const DirectX::XMFLOAT3 rotationEuler = cameraTransform.GetRotationEuler();

		SceneAssetPayload::Camera camera;
		camera.name = CookedCameraNameToString(cameraRecord, cameraIndex);
		camera.desc.position = cameraTransform.GetTranslation();
		camera.desc.pitchRadians = rotationEuler.x;
		camera.desc.yawRadians = rotationEuler.y;
		if (cameraRecord.verticalFovRadians > 0.0f)
		{
			camera.desc.fovYDegrees = cameraRecord.verticalFovRadians * 180.0f / DirectX::XM_PI;
		}
		camera.desc.nearZ = cameraRecord.nearPlane > 0.0f ? cameraRecord.nearPlane : camera.desc.nearZ;
		camera.desc.farZ = cameraRecord.farPlane > camera.desc.nearZ ? cameraRecord.farPlane : camera.desc.farZ;
		camera.desc.projectionKind = ToCameraProjectionKind(cameraRecord.projectionKind);
		return camera;
	}
}  // namespace Assets
