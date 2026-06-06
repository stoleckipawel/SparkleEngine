#include "PCH.h"

#include "Gltf/GltfCameraImporter.h"

#include <cgltf.h>

#include <algorithm>
#include <format>

namespace
{
	DirectX::XMMATRIX ConvertGltfMatrixToEngine(DirectX::FXMMATRIX matrix) noexcept
	{
		const DirectX::XMMATRIX handedness = DirectX::XMMatrixScaling(1.0f, 1.0f, -1.0f);
		return DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(handedness, matrix), handedness);
	}

	DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node)
	{
		DirectX::XMMATRIX worldTransform = DirectX::XMMatrixIdentity();

		const cgltf_node* nodeChain[64];
		int depth = 0;
		for (const cgltf_node* currentNode = node; currentNode != nullptr && depth < 64; currentNode = currentNode->parent)
		{
			nodeChain[depth++] = currentNode;
		}

		for (int chainIndex = depth - 1; chainIndex >= 0; --chainIndex)
		{
			float localMatrix[16];
			cgltf_node_transform_local(nodeChain[chainIndex], localMatrix);
			const DirectX::XMMATRIX localTransform =
			    DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(localMatrix));
			worldTransform = DirectX::XMMatrixMultiply(worldTransform, localTransform);
		}

		return ConvertGltfMatrixToEngine(worldTransform);
	}

	std::string ResolveCameraName(const cgltf_node& node, std::uint32_t nodeIndex)
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
}

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
		camera.name = ResolveCameraName(node, static_cast<std::uint32_t>(nodeIndex));
		camera.sourceNodeIndex = static_cast<std::uint32_t>(nodeIndex);
		DirectX::XMStoreFloat4x4(&camera.worldTransform, ComputeNodeWorldTransform(&node));

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
