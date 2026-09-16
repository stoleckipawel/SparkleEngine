#pragma once

#include "Meshes/GpuMeshHandle.h"
#include "Scene/Geometry/RenderMeshClassification.h"

#include <DirectXMath.h>
#include <cstdint>
#include <limits>

struct MeshDrawTransform final
{
	DirectX::XMFLOAT4X4 WorldMatrix = {};
	DirectX::XMFLOAT4X4 PreviousWorldMatrix = {};
	DirectX::XMFLOAT3X4 WorldInvTranspose = {};
};

struct MeshDrawSkinning final
{
	std::uint64_t SkeletonAssetId = 0;
	std::uint32_t JointMatrixOffset = (std::numeric_limits<std::uint32_t>::max)();
};

struct MeshDrawMorph final
{
	std::uint32_t WeightOffset = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t TargetCount = 0u;
	std::uint32_t VertexCount = 0u;
};

struct MeshDrawSourceIdentity final
{
	std::uint32_t GpuSceneSlot = 0;
	std::uint64_t MeshAssetId = 0;
	std::uint32_t MeshGeneration = 0;
};

struct MeshDrawGeometry final
{
	RenderMeshKind MeshKind = RenderMeshKind::Static;
	GpuMeshHandle Mesh;
	DirectX::XMFLOAT3 LocalBoundsMin = {};
	DirectX::XMFLOAT3 LocalBoundsMax = {};
	bool HasLocalBounds = false;
};

struct MeshDraw
{
	MeshDrawTransform Transform;
	std::uint32_t MaterialSlot = 0;
	MeshDrawSkinning Skinning;
	MeshDrawMorph Morph;
	MeshDrawSourceIdentity Source;
	MeshDrawGeometry Geometry;
};
