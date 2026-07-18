#include "PCH.h"
#include "World/WorldTransformConversion.h"

#include "GameFramework/Public/Scene/Transform.h"

namespace ECS::WorldTransformConversion
{
	LocalTransform ToLocal(const Transform& transform) noexcept
	{
		const DirectX::XMFLOAT3 rotation = transform.GetRotationEuler();
		LocalTransform local;
		local.Translation = transform.GetTranslation();
		local.Scale = transform.GetScale();
		DirectX::XMStoreFloat4(
		    &local.Rotation,
		    DirectX::XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z));
		return local;
	}

	Transform ToPublic(const LocalTransform& transform) noexcept
	{
		const DirectX::XMMATRIX matrix = DirectX::XMMatrixScaling(transform.Scale.x, transform.Scale.y, transform.Scale.z) *
		                                 DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&transform.Rotation)) *
		                                 DirectX::XMMatrixTranslation(
		                                     transform.Translation.x,
		                                     transform.Translation.y,
		                                     transform.Translation.z);
		return Transform(matrix);
	}

	WorldTransform BuildWorld(const LocalTransform& transform) noexcept
	{
		WorldTransform world;
		const DirectX::XMMATRIX matrix = ToPublic(transform).GetWorldMatrix();
		DirectX::XMStoreFloat4x4(&world.Matrix, matrix);
		DirectX::XMStoreFloat4x4(&world.InverseTranspose, DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, matrix)));
		return world;
	}
}
