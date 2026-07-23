#include "PCH.h"

#include "Gltf/GltfCameraImporter.h"

#include "Gltf/GltfNodeTransformUtils.h"

#include <cgltf.h>

#include <format>

class GltfCameraImporterOperations final
{
  public:
	static std::string ResolveCameraName(const cgltf_node& node, std::uint32_t nodeIndex)
	{
		if (node.name != nullptr && node.name[0] != '\0')
		{
			return node.name;
		}

		if (node.camera != nullptr && node.camera->name != nullptr && node.camera->name[0] != '\0')
		{
			return node.camera->name;
		}

		return std::format("glTF Camera {}", nodeIndex);
	}
};

void GltfCameraImporter::ImportCameras(const cgltf_data* data, SourceImportResult& result)
{
	if (data == nullptr)
	{
		return;
	}

	result.scene.cameras.reserve(data->cameras_count);
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (node.camera == nullptr)
		{
			continue;
		}

		ImportedCamera camera;
		camera.name = GltfCameraImporterOperations::ResolveCameraName(node, static_cast<std::uint32_t>(nodeIndex));
		camera.sourceNodeIndex = static_cast<std::uint32_t>(nodeIndex);
		DirectX::XMStoreFloat4x4(&camera.worldTransform, GltfNodeTransformUtils::ComputeNodeWorldTransform(&node));

		const cgltf_camera& sourceCamera = *node.camera;
		if (sourceCamera.type == cgltf_camera_type_perspective)
		{
			camera.projectionKind = ImportedCameraProjectionKind::Perspective;
			camera.verticalFovRadians = sourceCamera.data.perspective.yfov;
			camera.nearPlane = sourceCamera.data.perspective.znear > 0.0f ? sourceCamera.data.perspective.znear : camera.nearPlane;
			camera.farPlane =
			    sourceCamera.data.perspective.zfar > camera.nearPlane ? sourceCamera.data.perspective.zfar : camera.farPlane;
		}
		else if (sourceCamera.type == cgltf_camera_type_orthographic)
		{
			camera.projectionKind = ImportedCameraProjectionKind::Orthographic;
			camera.nearPlane = sourceCamera.data.orthographic.znear > 0.0f ? sourceCamera.data.orthographic.znear : camera.nearPlane;
			camera.farPlane =
			    sourceCamera.data.orthographic.zfar > camera.nearPlane ? sourceCamera.data.orthographic.zfar : camera.farPlane;
		}

		result.scene.cameras.push_back(std::move(camera));
	}
}
