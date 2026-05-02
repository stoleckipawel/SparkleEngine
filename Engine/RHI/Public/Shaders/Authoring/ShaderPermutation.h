#pragma once

#include "../../RHIAPI.h"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

struct ShaderPermutationValueDescriptor final
{
	std::string Name;
	std::string DefineValue;
};

struct ShaderPermutationDimensionDescriptor final
{
	std::string Name;
	std::string DefineName;
	std::uint32_t ValueCount = 1;
	std::vector<ShaderPermutationValueDescriptor> Values;
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
SPARKLE_RHI_API bool IsDefaultShaderPermutationVector(const ShaderPermutationVector& vector) noexcept;
SPARKLE_RHI_API std::vector<ShaderPermutationVector> EnumerateShaderPermutationVectors(const ShaderPermutationDomainDescriptor& domain);
SPARKLE_RHI_API std::vector<std::string> BuildShaderPermutationDefines(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector);
SPARKLE_RHI_API std::string BuildShaderPermutationVariantId(ShaderPermutationKey permutationKey);
SPARKLE_RHI_API std::string BuildShaderPermutationVariantId(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector);
SPARKLE_RHI_API std::string BuildShaderPermutationVectorName(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector);
SPARKLE_RHI_API ShaderPermutationDimensionDescriptor MakeShaderPermutationBoolDimension(
	std::string_view name,
	std::string_view defineName);
SPARKLE_RHI_API ShaderPermutationDimensionDescriptor MakeShaderPermutationEnumDimension(
	std::string_view name,
	std::string_view defineName,
	std::initializer_list<std::string_view> values);
SPARKLE_RHI_API std::uint32_t GetShaderPermutationValue(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector,
	std::string_view dimensionName) noexcept;

class ShaderPermutationBool
{
  public:
	static constexpr std::uint32_t kValueCount = 2;
	static constexpr std::string_view kName = "";
	static constexpr std::string_view kDefineName = "";

	static constexpr std::string_view GetValueName(std::uint32_t value) noexcept { return value == 0 ? "false" : "true"; }
	static constexpr std::string_view GetDefineValue(std::uint32_t value) noexcept { return value == 0 ? "0" : "1"; }
};

template <typename TEnum, std::uint32_t CountValue> class ShaderPermutationEnum
{
  public:
	using EnumType = TEnum;
	static constexpr std::uint32_t kValueCount = CountValue;
	static constexpr std::string_view kName = "";
	static constexpr std::string_view kDefineName = "";

	static std::string GetValueName(std::uint32_t value) { return std::to_string(value); }
	static std::string GetDefineValue(std::uint32_t value) { return std::to_string(value); }
};

template <typename TEnum> ShaderPermutationDimensionDescriptor MakeShaderPermutationEnumDimensionFromEnum(
	std::string_view name,
	std::string_view defineName)
{
	static_assert(std::is_enum_v<TEnum>, "SHADER_PERMUTATION_ENUM requires an enum type.");
	static_assert(requires { TEnum::Count; }, "SHADER_PERMUTATION_ENUM requires an enum class with a Count value.");

	ShaderPermutationDimensionDescriptor dimension;
	dimension.Name = std::string(name);
	dimension.DefineName = std::string(defineName);
	dimension.ValueCount = static_cast<std::uint32_t>(TEnum::Count);
	dimension.Values.reserve(dimension.ValueCount);
	for (std::uint32_t value = 0; value < dimension.ValueCount; ++value)
	{
		dimension.Values.push_back(ShaderPermutationValueDescriptor{
		    .Name = std::to_string(value),
		    .DefineValue = std::to_string(value)});
	}
	return dimension;
}

template <typename... TDimensions> class TShaderPermutationDomain
{
  public:
	static constexpr std::size_t kDimensionCount = sizeof...(TDimensions);

	TShaderPermutationDomain() = default;
	explicit TShaderPermutationDomain(const ShaderPermutationVector& vector) noexcept
	    : m_vector(&vector)
	{
	}

	static ShaderPermutationDomainDescriptor GetDescriptor()
	{
		ShaderPermutationDomainDescriptor descriptor;
		descriptor.Dimensions.reserve(kDimensionCount);
		(AppendDimension<TDimensions>(descriptor), ...);
		return descriptor;
	}

	static std::vector<ShaderPermutationVector> EnumerateVectors()
	{
		return EnumerateShaderPermutationVectors(GetDescriptor());
	}

	template <typename TDimension> std::uint32_t Get() const noexcept
	{
		constexpr std::size_t index = FindDimensionIndex<TDimension, TDimensions...>();
		if (m_vector == nullptr || index >= m_vector->Values.size())
		{
			return 0;
		}
		return m_vector->Values[index];
	}

  private:
	const ShaderPermutationVector* m_vector = nullptr;

	template <typename TDimension> static void AppendDimension(ShaderPermutationDomainDescriptor& descriptor)
	{
		std::vector<ShaderPermutationValueDescriptor> values;
		values.reserve(TDimension::kValueCount);
		for (std::uint32_t value = 0; value < TDimension::kValueCount; ++value)
		{
			values.push_back(ShaderPermutationValueDescriptor{
			    .Name = ReadValueName<TDimension>(value),
			    .DefineValue = ReadDefineValue<TDimension>(value)});
		}

		const std::string defineName(ReadDefineName<TDimension>());
		const std::string name(ReadDimensionName<TDimension>(defineName));
		descriptor.Dimensions.push_back(ShaderPermutationDimensionDescriptor{
		    .Name = name,
		    .DefineName = defineName,
		    .ValueCount = TDimension::kValueCount,
		    .Values = std::move(values),
		});
	}

	template <typename TDimension> static constexpr std::string_view ReadDefineName() noexcept
	{
		if constexpr (requires { TDimension::kDefineName; })
		{
			return TDimension::kDefineName;
		}
		else
		{
			return "";
		}
	}

	template <typename TDimension> static constexpr std::string_view ReadDimensionName(std::string_view defineName) noexcept
	{
		if constexpr (requires { TDimension::kName; })
		{
			if (!TDimension::kName.empty())
			{
				return TDimension::kName;
			}
		}
		return defineName.empty() ? std::string_view("PermutationDimension") : defineName;
	}

	template <typename TDimension> static std::string ReadValueName(std::uint32_t value)
	{
		if constexpr (requires { TDimension::GetValueName(value); })
		{
			return std::string(TDimension::GetValueName(value));
		}
		else
		{
			return std::to_string(value);
		}
	}

	template <typename TDimension> static std::string ReadDefineValue(std::uint32_t value)
	{
		if constexpr (requires { TDimension::GetDefineValue(value); })
		{
			return std::string(TDimension::GetDefineValue(value));
		}
		else
		{
			return std::to_string(value);
		}
	}

	template <typename TNeedle, typename TFirst, typename... TRest> static consteval std::size_t FindDimensionIndex()
	{
		if constexpr (std::is_same_v<TNeedle, TFirst>)
		{
			return 0;
		}
		else
		{
			static_assert(sizeof...(TRest) > 0, "Permutation dimension is not part of this shader domain.");
			return 1 + FindDimensionIndex<TNeedle, TRest...>();
		}
	}
};

#define BEGIN_SHADER_PERMUTATION_DOMAIN(DomainName) \
	class DomainName final \
	{ \
	  public: \
		static ::ShaderPermutationDomainDescriptor GetDescriptor() \
		{ \
			::ShaderPermutationDomainDescriptor descriptor;

#define SHADER_PERMUTATION_BOOL(DefineName) \
	descriptor.Dimensions.push_back(::MakeShaderPermutationBoolDimension(DefineName, DefineName));

#define SHADER_PERMUTATION_ENUM(EnumType, DefineName) \
	descriptor.Dimensions.push_back(::MakeShaderPermutationEnumDimensionFromEnum<EnumType>(DefineName, DefineName));

#define END_SHADER_PERMUTATION_DOMAIN() \
	return descriptor; \
	} \
	};