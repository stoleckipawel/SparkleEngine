#include "PCH.h"

#include "Fbx/FbxLightImporter.h"

#include "Fbx/FbxNodeTransformConverter.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Math/WorldCoordinateSystem.h"

#include <DirectXMath.h>

#include <algorithm>
#include <format>
#include <limits>

class FbxLightTranslation final
{
public:
	struct PhotometricProperties final
	{
		DirectX::XMFLOAT3 Color = {};
		float Illuminance = 0.0f;
		float LuminousIntensity = 0.0f;
		float Luminance = 0.0f;
		DirectX::XMFLOAT3 DistanceAttenuationCoefficients = {};
	};

	struct WorldPose final
	{
		DirectX::XMFLOAT4X4 Transform = {};
		DirectX::XMFLOAT3 Direction = {};
		DirectX::XMFLOAT3 Tangent = {};
	};

	static ImportedLightKind ToImportedLightKind(aiLightSourceType type) noexcept
	{
		switch (type)
		{
			case aiLightSource_DIRECTIONAL:
				return ImportedLightKind::Directional;
			case aiLightSource_POINT:
				return ImportedLightKind::Point;
			case aiLightSource_SPOT:
				return ImportedLightKind::Spot;
			case aiLightSource_AREA:
				return ImportedLightKind::Rect;
			case aiLightSource_AMBIENT:
			case aiLightSource_UNDEFINED:
			default:
				return ImportedLightKind::Unknown;
		}
	}

	static PhotometricProperties ComputePhotometricProperties(const aiLight& source, ImportedLightKind kind) noexcept
	{
		const float peak = (std::max) ({source.mColorDiffuse.r, source.mColorDiffuse.g, source.mColorDiffuse.b});
		PhotometricProperties properties;
		properties.Color = peak > 0.0f
		    ? DirectX::XMFLOAT3(source.mColorDiffuse.r / peak, source.mColorDiffuse.g / peak, source.mColorDiffuse.b / peak)
		    : DirectX::XMFLOAT3{};

		switch (kind)
		{
			case ImportedLightKind::Directional:
				properties.Illuminance = peak;
				break;
			case ImportedLightKind::Point:
			case ImportedLightKind::Spot:
				properties.LuminousIntensity = peak;
				properties.DistanceAttenuationCoefficients = {
				    source.mAttenuationConstant,
				    source.mAttenuationLinear,
				    source.mAttenuationQuadratic};
				break;
			case ImportedLightKind::Rect:
				properties.Luminance = peak;
				break;
			case ImportedLightKind::Unknown:
				break;
		}
		return properties;
	}

	static WorldPose ComputeWorldPose(const aiLight& source, ImportedLightKind kind, const aiNode& node, float sourceMetersPerUnit)
	{
		const aiVector3D position = aiVector3D(source.mPosition.x, source.mPosition.y, -source.mPosition.z) * sourceMetersPerUnit;
		const aiVector3D localDirection(source.mDirection.x, source.mDirection.y, -source.mDirection.z);
		const aiVector3D up(source.mUp.x, source.mUp.y, -source.mUp.z);
		WorldPose pose;
		pose.Transform = kind == ImportedLightKind::Point
		    ? FbxNodeTransformConverter::BuildNodeAttachedTranslation(node, position)
		    : FbxNodeTransformConverter::BuildNodeAttachedOrientation(node, position, localDirection, up);

		const DirectX::XMMATRIX lightWorld = DirectX::XMLoadFloat4x4(&pose.Transform);
		const DirectX::XMVECTOR direction = DirectX::XMVector3TransformNormal(
		    DirectX::XMVectorSet(WorldCoordinates::kForwardX, WorldCoordinates::kForwardY, WorldCoordinates::kForwardZ, 0.0f),
		    lightWorld);
		const DirectX::XMVECTOR tangent = DirectX::XMVector3TransformNormal(
		    DirectX::XMVectorSet(WorldCoordinates::kRightX, WorldCoordinates::kRightY, WorldCoordinates::kRightZ, 0.0f),
		    lightWorld);
		DirectX::XMStoreFloat3(&pose.Direction, DirectX::XMVector3Normalize(direction));
		DirectX::XMStoreFloat3(&pose.Tangent, DirectX::XMVector3Normalize(tangent));
		return pose;
	}
};

void FbxLightImporter::ImportLights(const aiScene& scene, float sourceMetersPerUnit, SourceImportOutput& output)
{
	output.scene.lights.reserve(scene.mNumLights);
	for (unsigned int lightIndex = 0; lightIndex < scene.mNumLights; ++lightIndex)
	{
		const aiLight* sourceLight = scene.mLights[lightIndex];
		const aiNode* node = sourceLight != nullptr ? FbxNodeTransformConverter::FindNode(scene, sourceLight->mName) : nullptr;
		const ImportedLightKind kind =
		    sourceLight != nullptr ? FbxLightTranslation::ToImportedLightKind(sourceLight->mType) : ImportedLightKind::Unknown;
		if (sourceLight == nullptr || sourceLight->mName.length == 0 || node == nullptr || kind == ImportedLightKind::Unknown)
		{
			throw Diagnostics::Error(std::format("FBX light {} has incomplete source data or an unsupported kind.", lightIndex));
		}

		ImportedLight light;
		light.name = sourceLight->mName.C_Str();
		light.kind = kind;
		light.sourceNodeIndex = FbxNodeTransformConverter::FindNodeIndex(scene, *node);
		light.innerAngleRadians = sourceLight->mAngleInnerCone;
		light.outerAngleRadians = sourceLight->mAngleOuterCone;
		light.width = sourceLight->mSize.x * sourceMetersPerUnit;
		light.height = sourceLight->mSize.y * sourceMetersPerUnit;
		if (light.sourceNodeIndex == (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error(std::format("FBX light '{}' has no source node index.", light.name));
		}
		const FbxLightTranslation::PhotometricProperties photometry =
		    FbxLightTranslation::ComputePhotometricProperties(*sourceLight, light.kind);
		light.color = photometry.Color;
		light.illuminance = photometry.Illuminance;
		light.luminousIntensity = photometry.LuminousIntensity;
		light.luminance = photometry.Luminance;
		light.distanceAttenuationCoefficients = photometry.DistanceAttenuationCoefficients;
		light.distanceAttenuationCoefficients.y /= sourceMetersPerUnit;
		light.distanceAttenuationCoefficients.z /= sourceMetersPerUnit * sourceMetersPerUnit;

		const FbxLightTranslation::WorldPose worldPose =
		    FbxLightTranslation::ComputeWorldPose(*sourceLight, light.kind, *node, sourceMetersPerUnit);
		light.worldTransform = worldPose.Transform;
		light.direction = worldPose.Direction;
		light.tangent = worldPose.Tangent;
		output.scene.lights.push_back(std::move(light));
	}
}
