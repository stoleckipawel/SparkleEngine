#pragma once

#include "../../RHIAPI.h"
#include "../GlobalShaderMap.h"
#include "ShaderParameterStruct.h"

#include <cassert>
#include <span>
#include <string_view>
#include <typeinfo>

struct ShaderRegistrationDesc final
{
	const std::type_info* ShaderType = nullptr;
	ShaderTypeId TypeId = 0;
	std::string_view ShaderName;
	std::string_view SourcePath;
	std::string_view EntryPoint;
	ShaderStage Stage = ShaderStage::Count;
	ShaderFeatureFlags Features = ShaderFeatureFlags::None;
	RayTracingShaderMetadata RayTracing = {};
	ShaderParameterStructDescriptor (*BuildParameterStructDescriptor)() = nullptr;
};

class SPARKLE_RHI_API GlobalShaderRegistry final
{
public:
	GlobalShaderRegistry() = delete;

	static void Register(ShaderRegistrationDesc desc);
	static std::span<const ShaderRegistrationDesc> GetRegistrations() noexcept;
	static const ShaderRegistrationDesc* FindByName(std::string_view shaderName) noexcept;
	static const ShaderRegistrationDesc* FindById(ShaderTypeId shaderType) noexcept;
	static const ShaderRegistrationDesc* FindByType(const std::type_info& shaderType) noexcept;
};

template <typename TShader> struct ShaderSourceMetadata
{
	static constexpr std::string_view kSourcePath = "";
	static constexpr std::string_view kEntryPoint = "";
	static constexpr ShaderStage kStage = ShaderStage::Count;
};

template <typename TShader> class GlobalShader
{
public:
	using ShaderType = TShader;

	static constexpr std::string_view GetShaderName(std::string_view defaultShaderName) noexcept
	{
		if constexpr (requires { TShader::kShaderName; })
		{
			return TShader::kShaderName;
		}
		return defaultShaderName;
	}

	static ShaderParameterStructDescriptor GetParameterStructDescriptor()
	{
		static_assert(requires { typename TShader::Parameters; });
		return TShader::Parameters::GetShaderParameterStructDescriptor();
	}

	static const ShaderRegistrationDesc& GetRegistration() noexcept
	{
		const ShaderRegistrationDesc* const registration = GlobalShaderRegistry::FindByType(typeid(TShader));
		assert(registration != nullptr && "Global shader type must be registered before use.");
		return *registration;
	}

	static constexpr ShaderFeatureFlags GetFeatures() noexcept
	{
		if constexpr (requires { TShader::kShaderFeatures; })
		{
			return TShader::kShaderFeatures;
		}
		return ShaderFeatureFlags::None;
	}

	static constexpr RayTracingShaderMetadata GetRayTracingMetadata() noexcept
	{
		if constexpr (requires { TShader::kRayTracingMetadata; })
		{
			return TShader::kRayTracingMetadata;
		}
		return {};
	}
};

template <typename TShader> class GlobalShaderAutoRegister final
{
public:
	GlobalShaderAutoRegister(std::string_view shaderName, std::string_view sourcePath, std::string_view entryPoint, ShaderStage stage)
	{
		const std::string_view resolvedName = GlobalShader<TShader>::GetShaderName(shaderName);
		GlobalShaderRegistry::Register(
		    ShaderRegistrationDesc{
		        .ShaderType = &typeid(TShader),
		        .TypeId = BuildShaderTypeId(resolvedName),
		        .ShaderName = resolvedName,
		        .SourcePath = sourcePath,
		        .EntryPoint = entryPoint,
		        .Stage = stage,
		        .Features = GlobalShader<TShader>::GetFeatures(),
		        .RayTracing = GlobalShader<TShader>::GetRayTracingMetadata(),
		        .BuildParameterStructDescriptor = []() -> ShaderParameterStructDescriptor (*)()
		        {
			        if constexpr (requires { typename TShader::Parameters; })
			        {
				        return &GlobalShader<TShader>::GetParameterStructDescriptor;
			        }
			        return nullptr;
		        }()});
	}
};

template <typename TShader> class ShaderRef final
{
public:
	ShaderRef() = default;

	static ShaderRef Resolve(const GlobalShaderMap& map, const CookedShaderLibrary& library, ShaderTarget target) noexcept
	{
		const ShaderRegistrationDesc& registration = GlobalShader<TShader>::GetRegistration();
		const GlobalShaderMapEntry* const entry =
		    map.GetPublicationHash() == library.GetPublicationHash() ? map.Find(registration.TypeId, target) : nullptr;
		const CookedShaderCodeRecord* const code = entry != nullptr ? library.Find(entry->CodeHash) : nullptr;
		ShaderRef ref;
		ref.m_shader = ResolvedShader{.Map = &map, .Library = &library, .Entry = entry, .Code = code};
		return ref;
	}

	const ResolvedShader& GetResolvedShader() const noexcept { return m_shader; }
	explicit operator bool() const noexcept { return m_shader.IsValid(); }

private:
	ResolvedShader m_shader;
};

#define IMPLEMENT_GLOBAL_SHADER(Class, Path, Entry, StageName)                             \
	template <> struct ShaderSourceMetadata<Class>                                         \
	{                                                                                      \
		static constexpr std::string_view kSourcePath = Path;                              \
		static constexpr std::string_view kEntryPoint = Entry;                             \
		static constexpr ::ShaderStage kStage = ::ShaderStage::StageName;                  \
	};                                                                                     \
	inline static const ::GlobalShaderAutoRegister<Class> AutoRegisterGlobalShader_##Class \
	{                                                                                      \
		#Class, Path, Entry, ::ShaderStage::StageName                                      \
	}
