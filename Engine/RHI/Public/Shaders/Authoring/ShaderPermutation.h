#pragma once

#include "../../RHIAPI.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

struct ShaderPermutationDimensionDescriptor final
{
	std::string Name;
	std::uint32_t ValueCount = 1;
};

struct ShaderPermutationDomainDescriptor final
{
	std::vector<ShaderPermutationDimensionDescriptor> Dimensions;
};

struct ShaderPermutationVector final
{
	std::vector<std::uint32_t> Values;
};

using ShaderPermutationKey = std::uint64_t;

SPARKLE_RHI_API ShaderPermutationKey BuildShaderPermutationKey(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector) noexcept;

class ShaderPermutationBool
{
  public:
	static constexpr std::uint32_t kValueCount = 2;
	static constexpr std::string_view kDefineName = "";
};

template <typename TEnum, std::uint32_t CountValue> class ShaderPermutationEnum
{
  public:
	using EnumType = TEnum;
	static constexpr std::uint32_t kValueCount = CountValue;
	static constexpr std::string_view kDefineName = "";
};

template <typename... TDimensions> class TShaderPermutationDomain final
{
  public:
	static constexpr std::size_t kDimensionCount = sizeof...(TDimensions);

	static ShaderPermutationDomainDescriptor GetDescriptor()
	{
		ShaderPermutationDomainDescriptor descriptor;
		descriptor.Dimensions.reserve(kDimensionCount);
		(AppendDimension<TDimensions>(descriptor), ...);
		return descriptor;
	}

	static std::vector<ShaderPermutationVector> EnumerateVectors()
	{
		ShaderPermutationDomainDescriptor descriptor = GetDescriptor();
		std::vector<ShaderPermutationVector> vectors;
		ShaderPermutationVector current;
		current.Values.resize(descriptor.Dimensions.size());
		EnumerateRecursive(descriptor, 0, current, vectors);
		return vectors;
	}

  private:
	template <typename TDimension> static void AppendDimension(ShaderPermutationDomainDescriptor& descriptor)
	{
		descriptor.Dimensions.push_back(ShaderPermutationDimensionDescriptor{
		    .Name = std::string(TDimension::kDefineName.empty() ? "PermutationDimension" : TDimension::kDefineName),
		    .ValueCount = TDimension::kValueCount,
		});
	}

	static void EnumerateRecursive(
		const ShaderPermutationDomainDescriptor& descriptor,
		std::size_t dimensionIndex,
		ShaderPermutationVector& current,
		std::vector<ShaderPermutationVector>& outVectors)
	{
		if (dimensionIndex >= descriptor.Dimensions.size())
		{
			outVectors.push_back(current);
			return;
		}

		const std::uint32_t valueCount = descriptor.Dimensions[dimensionIndex].ValueCount;
		for (std::uint32_t value = 0; value < valueCount; ++value)
		{
			current.Values[dimensionIndex] = value;
			EnumerateRecursive(descriptor, dimensionIndex + 1, current, outVectors);
		}
	}
};