#pragma once

#include <DirectXMath.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace SkeletonTransformContract
{
	template <typename JointRange> bool BuildEvaluationOrder(const JointRange& joints, std::vector<std::uint32_t>& order)
	{
		order.clear();
		order.reserve(joints.size());
		std::vector<std::uint8_t> states(joints.size(), 0u);
		std::vector<std::uint32_t> path;
		path.reserve(joints.size());

		for (std::uint32_t jointIndex = 0; jointIndex < joints.size(); ++jointIndex)
		{
			if (states[jointIndex] == 2u)
			{
				continue;
			}

			path.clear();
			std::uint32_t current = jointIndex;
			while (current < joints.size() && states[current] == 0u)
			{
				states[current] = 1u;
				path.push_back(current);
				const std::uint32_t parent = joints[current].parentJointIndex;
				if (parent != (std::numeric_limits<std::uint32_t>::max)() && parent >= joints.size())
				{
					return false;
				}
				current = parent;
			}
			if (current < joints.size() && states[current] == 1u)
			{
				return false;
			}

			while (!path.empty())
			{
				const std::uint32_t resolvedJoint = path.back();
				path.pop_back();
				states[resolvedJoint] = 2u;
				order.push_back(resolvedJoint);
			}
		}
		return order.size() == joints.size();
	}

	inline bool IsFinite(const DirectX::XMFLOAT4X4& matrix) noexcept
	{
		const float* elements = &matrix._11;
		for (std::uint32_t element = 0; element < 16u; ++element)
		{
			if (!std::isfinite(elements[element]))
			{
				return false;
			}
		}
		return true;
	}

	inline bool IsInvertible(const DirectX::XMFLOAT4X4& matrix, float epsilon = 1.0e-8f) noexcept
	{
		DirectX::XMVECTOR determinant;
		DirectX::XMMatrixInverse(&determinant, DirectX::XMLoadFloat4x4(&matrix));
		const float value = DirectX::XMVectorGetX(determinant);
		return std::isfinite(value) && std::abs(value) > epsilon;
	}

	inline bool IsTrsDecomposable(const DirectX::XMFLOAT4X4& matrix) noexcept
	{
		DirectX::XMVECTOR scale;
		DirectX::XMVECTOR rotation;
		DirectX::XMVECTOR translation;
		return DirectX::XMMatrixDecompose(&scale, &rotation, &translation, DirectX::XMLoadFloat4x4(&matrix))
		    && DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(rotation)) > 1.0e-8f;
	}

	inline bool MatricesNear(DirectX::FXMMATRIX lhs, DirectX::CXMMATRIX rhs, float epsilon = 2.0e-3f) noexcept
	{
		DirectX::XMFLOAT4X4 lhsValues;
		DirectX::XMFLOAT4X4 rhsValues;
		DirectX::XMStoreFloat4x4(&lhsValues, lhs);
		DirectX::XMStoreFloat4x4(&rhsValues, rhs);
		const float* lhsElements = &lhsValues._11;
		const float* rhsElements = &rhsValues._11;
		for (std::uint32_t element = 0; element < 16u; ++element)
		{
			if (std::abs(lhsElements[element] - rhsElements[element]) > epsilon)
			{
				return false;
			}
		}
		return true;
	}

	inline bool SatisfiesBindInvariant(
	    const DirectX::XMFLOAT4X4& bindLocal,
	    const DirectX::XMFLOAT4X4& parentSpace,
	    const DirectX::XMFLOAT4X4* parentBindModel,
	    const DirectX::XMFLOAT4X4& bindModel) noexcept
	{
		DirectX::XMMATRIX reconstructed = DirectX::XMLoadFloat4x4(&bindLocal) * DirectX::XMLoadFloat4x4(&parentSpace);
		if (parentBindModel != nullptr)
		{
			reconstructed *= DirectX::XMLoadFloat4x4(parentBindModel);
		}
		return MatricesNear(reconstructed, DirectX::XMLoadFloat4x4(&bindModel));
	}
}
