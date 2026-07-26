#include "PCH.h"

#include "SceneData/Preparation/RenderObjectPreparation.h"

#include "Core/Public/Math/Frustum.h"

#include <algorithm>
#include <array>

void RenderObjectPreparation::TransformRange(
    std::span<const ResolvedRenderObject> inputs,
    std::span<PreparedRenderObject> outputs) noexcept
{
	const std::size_t count =
	    (std::min)(inputs.size(), outputs.size());
	for (std::size_t index = 0u; index < count; ++index)
	{
		const ResolvedRenderObject& input = inputs[index];
		PreparedRenderObject& output = outputs[index];
		output = PreparedRenderObject{
		    .Object = input.Object,
		    .Draw = input.Draw,
		    .WorldBounds =
		        TransformBounds(
		            input.Draw.Geometry,
		            input.WorldMatrix),
		    .Material = input.Material,
		    .InstanceGroupIndex = input.InstanceGroupIndex,
		    .MaterialAlphaMode = input.MaterialAlphaMode};
		output.Draw.Transform =
		    MeshDrawTransform{
		        .WorldMatrix = input.WorldMatrix,
		        .PreviousWorldMatrix =
		            input.PreviousWorldMatrix,
		        .WorldInvTranspose =
		            input.WorldInverseTranspose};
	}
}

void RenderObjectPreparation::EvaluateVisibilityRange(
    const Frustum& frustum,
    const DirectX::XMFLOAT3& cameraPosition,
    std::span<PreparedRenderObject> objects) noexcept
{
	for (PreparedRenderObject& object : objects)
	{
		object.MaterialClassification =
		    ClassifyMaterial(object.MaterialAlphaMode);
		object.CameraDistanceSquared =
		    ComputeCameraDistanceSquared(
		        cameraPosition,
		        object.WorldBounds,
		        object.Draw.Transform.WorldMatrix);
		object.RasterVisible =
		    object.MaterialClassification !=
		        RenderMaterialClassification::Rejected &&
		    Intersects(frustum, object.WorldBounds);
	}
}

RenderMeshWorldBounds RenderObjectPreparation::TransformBounds(
    const MeshDrawGeometry& geometry,
    const DirectX::XMFLOAT4X4& worldMatrix) noexcept
{
	if (!geometry.HasLocalBounds)
	{
		const DirectX::XMFLOAT3 position{
		    worldMatrix._41,
		    worldMatrix._42,
		    worldMatrix._43};
		return RenderMeshWorldBounds{
		    .Min = position,
		    .Max = position,
		    .Valid = true};
	}

	const DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&worldMatrix);
	const std::array<DirectX::XMFLOAT3, 8> corners = BuildBoundsCorners(geometry);

	RenderMeshWorldBounds bounds;
	for (const DirectX::XMFLOAT3& corner : corners)
	{
		ExpandBounds(TransformPoint(corner, world), bounds);
	}
	return bounds;
}

std::array<DirectX::XMFLOAT3, 8> RenderObjectPreparation::BuildBoundsCorners(
    const MeshDrawGeometry& geometry) noexcept
{
	return {
	    DirectX::XMFLOAT3{
	        geometry.LocalBoundsMin.x,
	        geometry.LocalBoundsMin.y,
	        geometry.LocalBoundsMin.z},
	    DirectX::XMFLOAT3{
	        geometry.LocalBoundsMax.x,
	        geometry.LocalBoundsMin.y,
	        geometry.LocalBoundsMin.z},
	    DirectX::XMFLOAT3{
	        geometry.LocalBoundsMin.x,
	        geometry.LocalBoundsMax.y,
	        geometry.LocalBoundsMin.z},
	    DirectX::XMFLOAT3{
	        geometry.LocalBoundsMax.x,
	        geometry.LocalBoundsMax.y,
	        geometry.LocalBoundsMin.z},
	    DirectX::XMFLOAT3{
	        geometry.LocalBoundsMin.x,
	        geometry.LocalBoundsMin.y,
	        geometry.LocalBoundsMax.z},
	    DirectX::XMFLOAT3{
	        geometry.LocalBoundsMax.x,
	        geometry.LocalBoundsMin.y,
	        geometry.LocalBoundsMax.z},
	    DirectX::XMFLOAT3{
	        geometry.LocalBoundsMin.x,
	        geometry.LocalBoundsMax.y,
	        geometry.LocalBoundsMax.z},
	    DirectX::XMFLOAT3{
	        geometry.LocalBoundsMax.x,
	        geometry.LocalBoundsMax.y,
	        geometry.LocalBoundsMax.z}};
}

DirectX::XMFLOAT3 RenderObjectPreparation::TransformPoint(
    const DirectX::XMFLOAT3& point,
    DirectX::FXMMATRIX worldMatrix) noexcept
{
	DirectX::XMFLOAT3 transformed;
	DirectX::XMStoreFloat3(
	    &transformed,
	    DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&point), worldMatrix));
	return transformed;
}

void RenderObjectPreparation::ExpandBounds(
    const DirectX::XMFLOAT3& point,
    RenderMeshWorldBounds& bounds) noexcept
{
	if (!bounds.Valid)
	{
		bounds.Min = point;
		bounds.Max = point;
		bounds.Valid = true;
		return;
	}

	bounds.Min.x = (std::min)(bounds.Min.x, point.x);
	bounds.Min.y = (std::min)(bounds.Min.y, point.y);
	bounds.Min.z = (std::min)(bounds.Min.z, point.z);
	bounds.Max.x = (std::max)(bounds.Max.x, point.x);
	bounds.Max.y = (std::max)(bounds.Max.y, point.y);
	bounds.Max.z = (std::max)(bounds.Max.z, point.z);
}

bool RenderObjectPreparation::Intersects(
    const Frustum& frustum,
    const RenderMeshWorldBounds& bounds) noexcept
{
	if (!bounds.Valid)
	{
		return true;
	}

	for (const DirectX::XMFLOAT4& plane : frustum.planes)
	{
		const DirectX::XMFLOAT3 positive{
		    plane.x >= 0.0f ? bounds.Max.x : bounds.Min.x,
		    plane.y >= 0.0f ? bounds.Max.y : bounds.Min.y,
		    plane.z >= 0.0f ? bounds.Max.z : bounds.Min.z};
		if (plane.x * positive.x +
		        plane.y * positive.y +
		        plane.z * positive.z +
		        plane.w <
		    0.0f)
		{
			return false;
		}
	}
	return true;
}

RenderMaterialClassification
RenderObjectPreparation::ClassifyMaterial(
    std::uint32_t alphaMode) noexcept
{
	switch (alphaMode)
	{
		case 0u:
			return RenderMaterialClassification::Opaque;
		case 1u:
			return RenderMaterialClassification::AlphaTested;
		case 2u:
			return RenderMaterialClassification::Transparent;
		default:
			return RenderMaterialClassification::Rejected;
	}
}

float RenderObjectPreparation::ComputeCameraDistanceSquared(
    const DirectX::XMFLOAT3& cameraPosition,
    const RenderMeshWorldBounds& bounds,
    const DirectX::XMFLOAT4X4& worldMatrix) noexcept
{
	const DirectX::XMFLOAT3 center =
	    bounds.Valid
	        ? DirectX::XMFLOAT3{
	              0.5f * (bounds.Min.x + bounds.Max.x),
	              0.5f * (bounds.Min.y + bounds.Max.y),
	              0.5f * (bounds.Min.z + bounds.Max.z)}
	        : DirectX::XMFLOAT3{
	              worldMatrix._41,
	              worldMatrix._42,
	              worldMatrix._43};
	const float x = center.x - cameraPosition.x;
	const float y = center.y - cameraPosition.y;
	const float z = center.z - cameraPosition.z;
	return x * x + y * y + z * z;
}
