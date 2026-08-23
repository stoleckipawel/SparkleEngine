#pragma once

#include "../../RHIAPI.h"
#include "../CookedShaderPackage.h"
#include "../ShaderStage.h"
#include "ShaderParameterStruct.h"

#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <typeinfo>

struct ShaderRegistrationDesc final
{
	const std::type_info* ShaderType = nullptr;
	std::string_view ShaderName;
	std::string_view PackageName;
	std::string_view BindingLayoutId;
	std::string_view SourcePath;
	std::string_view EntryPoint;
	ShaderStage Stage = ShaderStage::Count;
	CookedShaderPackageKind PackageKind = CookedShaderPackageKind::Graphics;
	CookedShaderPackageFeatureFlags PackageFeatures = CookedShaderPackageFeatureFlags::None;
	CookedShaderRayTracingExportKind RayTracingExportKind = CookedShaderRayTracingExportKind::None;
	std::string_view RayTracingExportName;
	std::uint32_t RayTracingPayloadSizeInBytes = 0;
	std::uint32_t RayTracingAttributeSizeInBytes = 0;
	std::uint32_t RayTracingMaxRecursionDepth = 0;
	ShaderParameterStructDescriptor (*BuildParameterStructDescriptor)() = nullptr;
};

struct RayTracingHitGroupRegistrationDesc final
{
	std::string_view PackageName;
	std::string_view HitGroupName;
	std::string_view ClosestHitExportName;
	std::string_view AnyHitExportName;
	std::string_view IntersectionExportName;
};

SPARKLE_RHI_API std::string BuildShaderPackageIdFromSourcePath(std::string_view sourcePath);
SPARKLE_RHI_API std::string GetShaderRegistrationPackageId(const ShaderRegistrationDesc& shader);
SPARKLE_RHI_API std::string GetShaderRegistrationBindingLayoutId(const ShaderRegistrationDesc& shader);
SPARKLE_RHI_API CookedShaderPackageKind GetDefaultCookedShaderPackageKind(ShaderStage stage) noexcept;

class SPARKLE_RHI_API GlobalShaderRegistry final
{
public:
	GlobalShaderRegistry() = delete;

	static void Register(ShaderRegistrationDesc desc);
	static void RegisterRayTracingHitGroup(RayTracingHitGroupRegistrationDesc desc);
	static std::span<const ShaderRegistrationDesc> GetRegistrations() noexcept;
	static std::span<const RayTracingHitGroupRegistrationDesc> GetRayTracingHitGroups() noexcept;
	static const ShaderRegistrationDesc* FindByName(std::string_view shaderName) noexcept;
	static const ShaderRegistrationDesc* FindByType(const std::type_info& shaderType) noexcept;
};

template <typename TShader> struct ShaderSourceMetadata
{
	static constexpr std::string_view kPackageName = "";
	static constexpr std::string_view kBindingLayoutId = "";
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
		else
		{
			return defaultShaderName;
		}
	}
	static constexpr std::string_view GetShaderPackageName() noexcept
	{
		if constexpr (requires { TShader::kShaderPackageName; })
		{
			return TShader::kShaderPackageName;
		}
		else if constexpr (!ShaderSourceMetadata<TShader>::kPackageName.empty())
		{
			return ShaderSourceMetadata<TShader>::kPackageName;
		}
		else
		{
			return {};
		}
	}
	static constexpr std::string_view GetBindingLayoutId() noexcept
	{
		if constexpr (requires { TShader::kBindingLayoutId; })
		{
			return TShader::kBindingLayoutId;
		}
		else if constexpr (!ShaderSourceMetadata<TShader>::kBindingLayoutId.empty())
		{
			return ShaderSourceMetadata<TShader>::kBindingLayoutId;
		}
		else
		{
			return {};
		}
	}
	static ShaderParameterStructDescriptor GetParameterStructDescriptor()
	{
		return TShader::Parameters::GetShaderParameterStructDescriptor();
	}
	static const ShaderRegistrationDesc& GetRegistration() noexcept
	{
		const ShaderRegistrationDesc* const registration = GlobalShaderRegistry::FindByType(typeid(TShader));
		assert(registration != nullptr && "Global shader type must be registered before use.");
		return *registration;
	}
	static constexpr CookedShaderPackageFeatureFlags GetPackageFeatures() noexcept
	{
		if constexpr (requires { TShader::kPackageFeatures; })
		{
			return TShader::kPackageFeatures;
		}
		else
		{
			return CookedShaderPackageFeatureFlags::None;
		}
	}
};

template <typename TShader> class GlobalShaderAutoRegister final
{
public:
	GlobalShaderAutoRegister(
	    std::string_view shaderName,
	    std::string_view sourcePath,
	    std::string_view entryPoint,
	    ShaderStage stage,
	    CookedShaderPackageFeatureFlags packageFeatures = GlobalShader<TShader>::GetPackageFeatures())
	{
		GlobalShaderRegistry::Register(
		    ShaderRegistrationDesc{
		        .ShaderType = &typeid(TShader),
		        .ShaderName = GlobalShader<TShader>::GetShaderName(shaderName),
		        .PackageName = GlobalShader<TShader>::GetShaderPackageName(),
		        .BindingLayoutId = GlobalShader<TShader>::GetBindingLayoutId(),
		        .SourcePath = sourcePath,
		        .EntryPoint = entryPoint,
		        .Stage = stage,
		        .PackageKind = GetDefaultCookedShaderPackageKind(stage),
		        .PackageFeatures = packageFeatures,
		        .BuildParameterStructDescriptor = &GlobalShader<TShader>::GetParameterStructDescriptor,
		    });
	}
};

template <typename TShader> class RayTracingShaderAutoRegister final
{
public:
	RayTracingShaderAutoRegister(std::string_view shaderName, std::string_view sourcePath, std::string_view entryPoint)
	{
		GlobalShaderRegistry::Register(
		    ShaderRegistrationDesc{
		        .ShaderType = &typeid(TShader),
		        .ShaderName = GlobalShader<TShader>::GetShaderName(shaderName),
		        .PackageName = GlobalShader<TShader>::GetShaderPackageName(),
		        .BindingLayoutId = GlobalShader<TShader>::GetBindingLayoutId(),
		        .SourcePath = sourcePath,
		        .EntryPoint = entryPoint,
		        .Stage = ShaderStage::Count,
		        .PackageKind = CookedShaderPackageKind::RayTracingLibrary,
		        .PackageFeatures = GlobalShader<TShader>::GetPackageFeatures() | CookedShaderPackageFeatureFlags::UsesAccelerationStructure,
		        .RayTracingExportKind = TShader::kRayTracingExportKind,
		        .RayTracingExportName = entryPoint,
		        .RayTracingPayloadSizeInBytes = TShader::kRayTracingPayloadSizeInBytes,
		        .RayTracingAttributeSizeInBytes = TShader::kRayTracingAttributeSizeInBytes,
		        .RayTracingMaxRecursionDepth = TShader::kRayTracingMaxRecursionDepth,
		        .BuildParameterStructDescriptor = &GlobalShader<TShader>::GetParameterStructDescriptor,
		    });
	}
};

template <typename THitGroup> class RayTracingHitGroupAutoRegister final
{
public:
	RayTracingHitGroupAutoRegister()
	{
		static_assert(requires { typename THitGroup::ClosestHit; }, "Ray tracing hit groups require a ClosestHit shader type.");
		using ClosestHit = typename THitGroup::ClosestHit;
		constexpr std::string_view closestHitExportName = ShaderSourceMetadata<ClosestHit>::kEntryPoint;
		static_assert(
		    !closestHitExportName.empty(),
		    "Ray tracing hit group closest-hit shader must be registered with IMPLEMENT_RAY_TRACING_SHADER before the hit group.");

		std::string_view anyHitExportName;
		if constexpr (requires { typename THitGroup::AnyHit; })
		{
			using AnyHit = typename THitGroup::AnyHit;
			anyHitExportName = ShaderSourceMetadata<AnyHit>::kEntryPoint;
		}

		std::string_view intersectionExportName;
		if constexpr (requires { typename THitGroup::Intersection; })
		{
			using Intersection = typename THitGroup::Intersection;
			intersectionExportName = ShaderSourceMetadata<Intersection>::kEntryPoint;
		}

		GlobalShaderRegistry::RegisterRayTracingHitGroup(
		    RayTracingHitGroupRegistrationDesc{
		        .PackageName = THitGroup::kShaderPackageName,
		        .HitGroupName = THitGroup::kHitGroupName,
		        .ClosestHitExportName = closestHitExportName,
		        .AnyHitExportName = anyHitExportName,
		        .IntersectionExportName = intersectionExportName});
	}
};

template <typename TShader> class ShaderRef final
{
public:
	ShaderRef() = default;

	static ShaderRef Get() noexcept
	{
		const ShaderRegistrationDesc& registration = GlobalShader<TShader>::GetRegistration();
		ShaderRef ref;
		ref.m_sourcePath = registration.SourcePath;
		ref.m_entryPoint = registration.EntryPoint;
		ref.m_stage = registration.Stage;
		return ref;
	}

	std::string_view GetSourcePath() const noexcept { return m_sourcePath; }
	std::string_view GetEntryPoint() const noexcept { return m_entryPoint; }
	ShaderStage GetStage() const noexcept { return m_stage; }
	std::string GetPackageId() const
	{
		const ShaderRegistrationDesc& registration = GlobalShader<TShader>::GetRegistration();
		return registration.PackageName.empty() ? BuildShaderPackageIdFromSourcePath(m_sourcePath) : std::string(registration.PackageName);
	}
	explicit operator bool() const noexcept { return !m_sourcePath.empty() && !m_entryPoint.empty(); }

private:
	std::string_view m_sourcePath;
	std::string_view m_entryPoint;
	ShaderStage m_stage = ShaderStage::Count;
};

#define IMPLEMENT_GLOBAL_SHADER(Class, Path, Entry, StageName)                             \
	template <> struct ShaderSourceMetadata<Class>                                         \
	{                                                                                      \
		static constexpr std::string_view kPackageName = "";                               \
		static constexpr std::string_view kBindingLayoutId = "";                           \
		static constexpr std::string_view kSourcePath = Path;                              \
		static constexpr std::string_view kEntryPoint = Entry;                             \
		static constexpr ::ShaderStage kStage = ::ShaderStage::StageName;                  \
	};                                                                                     \
	inline static const ::GlobalShaderAutoRegister<Class> AutoRegisterGlobalShader_##Class \
	{                                                                                      \
		#Class, Path, Entry, ::ShaderStage::StageName                                      \
	}

#define IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(Class, Package, Path, Entry, StageName)         \
	template <> struct ShaderSourceMetadata<Class>                                         \
	{                                                                                      \
		static constexpr std::string_view kPackageName = Package;                          \
		static constexpr std::string_view kBindingLayoutId = Package;                      \
		static constexpr std::string_view kSourcePath = Path;                              \
		static constexpr std::string_view kEntryPoint = Entry;                             \
		static constexpr ::ShaderStage kStage = ::ShaderStage::StageName;                  \
	};                                                                                     \
	inline static const ::GlobalShaderAutoRegister<Class> AutoRegisterGlobalShader_##Class \
	{                                                                                      \
		#Class, Path, Entry, ::ShaderStage::StageName                                      \
	}

#define IMPLEMENT_RAY_TRACING_SHADER(Class, Path, Entry)                                           \
	template <> struct ShaderSourceMetadata<Class>                                                 \
	{                                                                                              \
		static constexpr std::string_view kPackageName = "";                                       \
		static constexpr std::string_view kBindingLayoutId = "";                                   \
		static constexpr std::string_view kSourcePath = Path;                                      \
		static constexpr std::string_view kEntryPoint = Entry;                                     \
		static constexpr ::ShaderStage kStage = ::ShaderStage::Count;                              \
	};                                                                                             \
	inline static const ::RayTracingShaderAutoRegister<Class> AutoRegisterRayTracingShader_##Class \
	{                                                                                              \
		#Class, Path, Entry                                                                        \
	}

#define IMPLEMENT_RAY_TRACING_HIT_GROUP(Class)                                                         \
	inline static const ::RayTracingHitGroupAutoRegister<Class> AutoRegisterRayTracingHitGroup_##Class \
	{                                                                                                  \
	}
