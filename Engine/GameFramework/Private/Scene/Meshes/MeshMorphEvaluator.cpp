#include "PCH.h"

#include "Scene/Meshes/MeshMorphEvaluator.h"

#include <algorithm>

namespace
{
	void AddWeighted(DirectX::XMFLOAT3& value, const DirectX::XMFLOAT3& delta, float weight) noexcept
	{
		value.x += delta.x * weight;
		value.y += delta.y * weight;
		value.z += delta.z * weight;
	}

	void AddWeightedTangent(DirectX::XMFLOAT4& value, const DirectX::XMFLOAT3& delta, float weight) noexcept
	{
		value.x += delta.x * weight;
		value.y += delta.y * weight;
		value.z += delta.z * weight;
	}
}

namespace MeshMorphEvaluator
{
	void ApplyWeights(MeshData& meshData, const MeshMorphData& morphTargets, std::span<const float> weights) noexcept
	{
		if (!morphTargets.HasTargets() || weights.empty())
		{
			return;
		}

		const std::size_t targetCount = (std::min)(morphTargets.targets.size(), weights.size());
		for (std::size_t targetIndex = 0; targetIndex < targetCount; ++targetIndex)
		{
			const float weight = weights[targetIndex];
			if (weight == 0.0f)
			{
				continue;
			}

			const MeshMorphTarget& target = morphTargets.targets[targetIndex];
			if (!target.IsValidForVertexCount(meshData.GetVertexCount()))
			{
				continue;
			}

			for (std::size_t vertexIndex = 0; vertexIndex < meshData.vertices.size(); ++vertexIndex)
			{
				VertexData& vertex = meshData.vertices[vertexIndex];
				const MeshMorphTargetDelta& delta = target.deltas[vertexIndex];
				AddWeighted(vertex.position, delta.position, weight);
				AddWeighted(vertex.normal, delta.normal, weight);
				AddWeightedTangent(vertex.tangent, delta.tangent, weight);
			}
		}
	}
}
