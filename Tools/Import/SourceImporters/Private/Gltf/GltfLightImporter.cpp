#include "PCH.h"

#include "Gltf/GltfLightImporter.h"

#include "Gltf/GltfNodeTransformUtils.h"

#include <cgltf.h>

#include <format>
#include <utility>

class GltfLightImporterOperations final
{
  public:
	static ImportedLightKind ToImportedLightKind(cgltf_light_type lightType) noexcept
	{
		switch (lightType)
		{
			case cgltf_light_type_directional:
				return ImportedLightKind::Directional;
			case cgltf_light_type_point:
				return ImportedLightKind::Point;
			case cgltf_light_type_spot:
				return ImportedLightKind::Spot;
			case cgltf_light_type_invalid:
			default:
				return ImportedLightKind::Unknown;
		}
	}

	static std::string ResolveLightName(const cgltf_node& node, const cgltf_light& light, std::uint32_t nodeIndex)
	{
		if (node.name != nullptr && node.name[0] != '\0')
		{
			return node.name;
		}

		if (light.name != nullptr && light.name[0] != '\0')
		{
			return light.name;
		}

		return std::format("glTF Light {}", nodeIndex);
	}
};

void GltfLightImporter::ImportLights(const cgltf_data* data, SourceImportResult& result)
{
	if (data == nullptr)
	{
		return;
	}

	result.scene.lights.reserve(data->lights_count);
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (node.light == nullptr)
		{
			continue;
		}

		const cgltf_light& sourceLight = *node.light;
		const DirectX::XMMATRIX worldTransform = GltfNodeTransformUtils::ComputeNodeWorldTransform(&node);

		ImportedLight light;
		light.name = GltfLightImporterOperations::ResolveLightName(node, sourceLight, static_cast<std::uint32_t>(nodeIndex));
		light.kind = GltfLightImporterOperations::ToImportedLightKind(sourceLight.type);
		light.color = {sourceLight.color[0], sourceLight.color[1], sourceLight.color[2]};
		light.intensity = sourceLight.intensity;
		light.range = sourceLight.range;
		light.innerConeAngleRadians = sourceLight.spot_inner_cone_angle;
		light.outerConeAngleRadians = sourceLight.spot_outer_cone_angle;
		light.sourceNodeIndex = static_cast<std::uint32_t>(nodeIndex);
		light.direction = GltfNodeTransformUtils::TransformDirection(worldTransform, {0.0f, 0.0f, -1.0f});
		DirectX::XMStoreFloat4x4(&light.worldTransform, worldTransform);
		result.scene.lights.push_back(std::move(light));
	}
}
