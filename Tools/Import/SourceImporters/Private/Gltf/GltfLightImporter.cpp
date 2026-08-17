#include "PCH.h"

#include "Gltf/GltfLightImporter.h"

#include "Gltf/GltfCoordinateConverter.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Math/WorldCoordinateSystem.h"

#include <cgltf.h>

#include <DirectXMath.h>

#include <format>
#include <utility>

class GltfLightTranslation final
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

	static std::string ResolveLightName(const cgltf_node& node, const cgltf_light& light)
	{
		if (node.name != nullptr && node.name[0] != '\0')
		{
			return node.name;
		}

		if (light.name != nullptr && light.name[0] != '\0')
		{
			return light.name;
		}

		return {};
	}
};

void GltfLightImporter::ImportLights(const cgltf_data* data, SourceImportOutput& output)
{
	if (data == nullptr)
	{
		throw Diagnostics::Error("glTF light import has no parsed scene.");
	}

	output.scene.lights.reserve(data->lights_count);
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (node.light == nullptr)
		{
			continue;
		}

		const cgltf_light& sourceLight = *node.light;
		const DirectX::XMMATRIX worldTransform =
		    GltfCoordinateConverter::ConvertCameraOrLightWorldTransform(GltfCoordinateConverter::ComputeNodeWorldTransform(&node));

		ImportedLight light;
		light.name = GltfLightTranslation::ResolveLightName(node, sourceLight);
		light.kind = GltfLightTranslation::ToImportedLightKind(sourceLight.type);
		if (light.kind == ImportedLightKind::Unknown)
		{
			throw Diagnostics::Error(std::format("glTF light {} uses an unsupported light kind.", nodeIndex));
		}
		light.color = {sourceLight.color[0], sourceLight.color[1], sourceLight.color[2]};
		if (light.IsDirectional())
		{
			light.illuminance = sourceLight.intensity;
		}
		else
		{
			light.luminousIntensity = sourceLight.intensity;
		}
		light.range = sourceLight.range;
		light.innerAngleRadians = sourceLight.spot_inner_cone_angle;
		light.outerAngleRadians = sourceLight.spot_outer_cone_angle;
		light.sourceNodeIndex = static_cast<std::uint32_t>(nodeIndex);
		light.direction = GltfCoordinateConverter::TransformDirection(
		    worldTransform,
		    {WorldCoordinates::kForwardX, WorldCoordinates::kForwardY, WorldCoordinates::kForwardZ});
		DirectX::XMStoreFloat4x4(&light.worldTransform, worldTransform);
		output.scene.lights.push_back(std::move(light));
	}
}
