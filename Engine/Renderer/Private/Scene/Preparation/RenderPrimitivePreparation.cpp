#include "PCH.h"

#include "Scene/Preparation/RenderPrimitivePreparation.h"

#include <algorithm>
#include <array>

void RenderPrimitivePreparation::TransformRange(
    std::span<const ResolvedRenderPrimitive> inputs,
    std::span<PreparedRenderPrimitive> outputs) noexcept
{
	const std::size_t count = (std::min) (inputs.size(), outputs.size());
	for (std::size_t index = 0u; index < count; ++index)
	{
		const ResolvedRenderPrimitive& input = inputs[index];
		PreparedRenderPrimitive& output = outputs[index];
		output = PreparedRenderPrimitive{
		    .Object = input.Object,
		    .Draw = input.Draw,
		    .WorldBounds = TransformBounds(input.Draw.Geometry, input.WorldMatrix),
		    .Material = input.Material,
		    .InstanceGroupIndex = input.InstanceGroupIndex,
		    .MaterialAlphaMode = input.MaterialAlphaMode};
		output.Draw.Transform = MeshDrawTransform{
		    .WorldMatrix = input.WorldMatrix,
		    .PreviousWorldMatrix = input.PreviousWorldMatrix,
		    .WorldInvTranspose = input.WorldInverseTranspose};
	}
}

RenderMeshWorldBounds RenderPrimitivePreparation::TransformBounds(
    const MeshDrawGeometry& geometry,
    const DirectX::XMFLOAT4X4& worldMatrix) noexcept
{
	if (!geometry.HasLocalBounds)
	{
		const DirectX::XMFLOAT3 position{worldMatrix._41, worldMatrix._42, worldMatrix._43};
		return RenderMeshWorldBounds{.Min = position, .Max = position, .Valid = true};
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

std::array<DirectX::XMFLOAT3, 8> RenderPrimitivePreparation::BuildBoundsCorners(const MeshDrawGeometry& geometry) noexcept
{
	return {
	    DirectX::XMFLOAT3{geometry.LocalBoundsMin.x, geometry.LocalBoundsMin.y, geometry.LocalBoundsMin.z},
	    DirectX::XMFLOAT3{geometry.LocalBoundsMax.x, geometry.LocalBoundsMin.y, geometry.LocalBoundsMin.z},
	    DirectX::XMFLOAT3{geometry.LocalBoundsMin.x, geometry.LocalBoundsMax.y, geometry.LocalBoundsMin.z},
	    DirectX::XMFLOAT3{geometry.LocalBoundsMax.x, geometry.LocalBoundsMax.y, geometry.LocalBoundsMin.z},
	    DirectX::XMFLOAT3{geometry.LocalBoundsMin.x, geometry.LocalBoundsMin.y, geometry.LocalBoundsMax.z},
	    DirectX::XMFLOAT3{geometry.LocalBoundsMax.x, geometry.LocalBoundsMin.y, geometry.LocalBoundsMax.z},
	    DirectX::XMFLOAT3{geometry.LocalBoundsMin.x, geometry.LocalBoundsMax.y, geometry.LocalBoundsMax.z},
	    DirectX::XMFLOAT3{geometry.LocalBoundsMax.x, geometry.LocalBoundsMax.y, geometry.LocalBoundsMax.z}};
}

DirectX::XMFLOAT3 RenderPrimitivePreparation::TransformPoint(const DirectX::XMFLOAT3& point, DirectX::FXMMATRIX worldMatrix) noexcept
{
	DirectX::XMFLOAT3 transformed;
	DirectX::XMStoreFloat3(&transformed, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&point), worldMatrix));
	return transformed;
}

void RenderPrimitivePreparation::ExpandBounds(const DirectX::XMFLOAT3& point, RenderMeshWorldBounds& bounds) noexcept
{
	if (!bounds.Valid)
	{
		bounds.Min = point;
		bounds.Max = point;
		bounds.Valid = true;
		return;
	}

	bounds.Min.x = (std::min) (bounds.Min.x, point.x);
	bounds.Min.y = (std::min) (bounds.Min.y, point.y);
	bounds.Min.z = (std::min) (bounds.Min.z, point.z);
	bounds.Max.x = (std::max) (bounds.Max.x, point.x);
	bounds.Max.y = (std::max) (bounds.Max.y, point.y);
	bounds.Max.z = (std::max) (bounds.Max.z, point.z);
}
