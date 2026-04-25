#pragma once

#include "../../RHIAPI.h"
#include "../ShaderReflection.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

struct ShaderParameterStructFieldDescriptor final
{
	std::string Name;
	CookedShaderResourceKind Kind = CookedShaderResourceKind::Unknown;
	CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ValueSizeInBytes = 0;
	std::uint32_t ValueAlignmentInBytes = 0;
};

struct ShaderParameterStructDescriptor final
{
	std::string Name;
	std::vector<ShaderParameterStructFieldDescriptor> Fields;

	bool IsEmpty() const noexcept { return Fields.empty(); }
};

template <typename TParameters> class TShaderParameterStructRegistry final
{
  public:
	TShaderParameterStructRegistry() = delete;

	static void AddField(ShaderParameterStructFieldDescriptor field)
	{
		std::vector<ShaderParameterStructFieldDescriptor>& fields = MutableFields();
		for (const ShaderParameterStructFieldDescriptor& existing : fields)
		{
			if (existing.Name == field.Name)
			{
				return;
			}
		}

		fields.push_back(std::move(field));
	}

	static ShaderParameterStructDescriptor BuildDescriptor(std::string_view name)
	{
		ShaderParameterStructDescriptor descriptor;
		descriptor.Name.assign(name);
		descriptor.Fields = MutableFields();
		return descriptor;
	}

  private:
	static std::vector<ShaderParameterStructFieldDescriptor>& MutableFields()
	{
		static std::vector<ShaderParameterStructFieldDescriptor> fields;
		return fields;
	}
};

template <typename TParameters> class TShaderParameterFieldAutoRegister final
{
  public:
	TShaderParameterFieldAutoRegister(
		std::string_view name,
		CookedShaderResourceKind kind,
		CookedShaderResourceDimension dimension,
		std::uint32_t arrayCount,
		std::uint32_t valueSizeInBytes,
		std::uint32_t valueAlignmentInBytes)
	{
		TShaderParameterStructRegistry<TParameters>::AddField(ShaderParameterStructFieldDescriptor{
		    .Name = std::string(name),
		    .Kind = kind,
		    .Dimension = dimension,
		    .ArrayCount = arrayCount,
		    .ValueSizeInBytes = valueSizeInBytes,
		    .ValueAlignmentInBytes = valueAlignmentInBytes,
		});
	}
};

struct Texture2D final
{
};

struct Texture3D final
{
};

struct TextureCube final
{
};

struct RWTexture2D final
{
};

struct SamplerState final
{
};

template <typename TValue = void> struct StructuredBuffer final
{
	using ValueType = TValue;
};

template <typename TValue = void> struct RWStructuredBuffer final
{
	using ValueType = TValue;
};

SPARKLE_RHI_API std::string BuildShaderParameterStructReport(const ShaderParameterStructDescriptor& descriptor);

#define BEGIN_SHADER_PARAMETER_STRUCT(StructName, Prefix) \
	struct StructName \
	{ \
		using ThisShaderParameterStruct = StructName; \
		static ::ShaderParameterStructDescriptor GetShaderParameterStructDescriptor() \
		{ \
			return ::TShaderParameterStructRegistry<ThisShaderParameterStruct>::BuildDescriptor(#StructName); \
		}

#define SHADER_PARAMETER(Type, Name) \
	Type Name{}; \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name, \
	    ::CookedShaderResourceKind::ConstantBuffer, \
	    ::CookedShaderResourceDimension::Buffer, \
	    1u, \
	    static_cast<std::uint32_t>(sizeof(Type)), \
	    static_cast<std::uint32_t>(alignof(Type))};

#define SHADER_PARAMETER_TEXTURE(Type, Name) \
	::Type Name{}; \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name, \
	    ::CookedShaderResourceKind::Texture, \
	    ::CookedShaderResourceDimension::Texture2D, \
	    1u, \
	    0u, \
	    0u};

#define SHADER_PARAMETER_SAMPLER(Type, Name) \
	::Type Name{}; \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name, \
	    ::CookedShaderResourceKind::Sampler, \
	    ::CookedShaderResourceDimension::Unknown, \
	    1u, \
	    0u, \
	    0u};

#define SHADER_PARAMETER_RDG_BUFFER_SRV(Type, Name) \
	::StructuredBuffer<Type> Name{}; \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name, \
	    ::CookedShaderResourceKind::StructuredBuffer, \
	    ::CookedShaderResourceDimension::Buffer, \
	    1u, \
	    0u, \
	    0u};

#define SHADER_PARAMETER_RDG_BUFFER_UAV(Type, Name) \
	::RWStructuredBuffer<Type> Name{}; \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name, \
	    ::CookedShaderResourceKind::RWStructuredBuffer, \
	    ::CookedShaderResourceDimension::Buffer, \
	    1u, \
	    0u, \
	    0u};

#define SHADER_PARAMETER_RDG_TEXTURE_SRV(Type, Name) SHADER_PARAMETER_TEXTURE(Texture2D, Name)

#define SHADER_PARAMETER_RDG_TEXTURE_UAV(Type, Name) \
	::RWTexture2D Name{}; \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name, \
	    ::CookedShaderResourceKind::RWTexture, \
	    ::CookedShaderResourceDimension::Texture2D, \
	    1u, \
	    0u, \
	    0u};

#define END_SHADER_PARAMETER_STRUCT() \
	};