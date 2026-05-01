#pragma once

#include "../../RHIAPI.h"
#include "../CookedShaderPackage.h"
#include "../ShaderStage.h"
#include "ShaderParameterStruct.h"
#include "ShaderPermutation.h"

#include <cstdint>
#include <span>
#include <string_view>

struct ShaderRegistrationDesc final
{
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
	ShaderPermutationDomainDescriptor (*BuildPermutationDomainDescriptor)() = nullptr;
};

struct RayTracingHitGroupRegistrationDesc final
{
	std::string_view PackageName;
	std::string_view HitGroupName;
	std::string_view ClosestHitShaderName;
	std::string_view AnyHitShaderName;
	std::string_view IntersectionShaderName;
};

constexpr CookedShaderPackageKind GetDefaultCookedShaderPackageKind(ShaderStage stage) noexcept
{
	return stage == ShaderStage::Compute ? CookedShaderPackageKind::Compute : CookedShaderPackageKind::Graphics;
}

class SPARKLE_RHI_API GlobalShaderRegistry final
{
  public:
	GlobalShaderRegistry() = delete;

	static void Register(ShaderRegistrationDesc desc);
	static void RegisterRayTracingHitGroup(RayTracingHitGroupRegistrationDesc desc);
	static std::span<const ShaderRegistrationDesc> GetRegistrations() noexcept;
	static std::span<const RayTracingHitGroupRegistrationDesc> GetRayTracingHitGroups() noexcept;
	static const ShaderRegistrationDesc* FindByName(std::string_view shaderName) noexcept;
};

class FGlobalShader
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = CookedShaderPackageFeatureFlags::None;
};

class FComputeShader : public FGlobalShader
{
};

class FRayTracingShader : public FGlobalShader
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = CookedShaderPackageFeatureFlags::UsesAccelerationStructure;
	static constexpr std::uint32_t kRayTracingPayloadSizeInBytes = 0;
	static constexpr std::uint32_t kRayTracingAttributeSizeInBytes = 0;
	static constexpr std::uint32_t kRayTracingMaxRecursionDepth = 0;
};

class FRayGenerationShader : public FRayTracingShader
{
  public:
	static constexpr CookedShaderRayTracingExportKind kRayTracingExportKind = CookedShaderRayTracingExportKind::RayGeneration;
};

class FMissShader : public FRayTracingShader
{
  public:
	static constexpr CookedShaderRayTracingExportKind kRayTracingExportKind = CookedShaderRayTracingExportKind::Miss;
};

class FClosestHitShader : public FRayTracingShader
{
  public:
	static constexpr CookedShaderRayTracingExportKind kRayTracingExportKind = CookedShaderRayTracingExportKind::ClosestHit;
};

class FAnyHitShader : public FRayTracingShader
{
  public:
	static constexpr CookedShaderRayTracingExportKind kRayTracingExportKind = CookedShaderRayTracingExportKind::AnyHit;
};

class FIntersectionShader : public FRayTracingShader
{
  public:
	static constexpr CookedShaderRayTracingExportKind kRayTracingExportKind = CookedShaderRayTracingExportKind::Intersection;
};

class FCallableShader : public FRayTracingShader
{
  public:
	static constexpr CookedShaderRayTracingExportKind kRayTracingExportKind = CookedShaderRayTracingExportKind::Callable;
};

class FRayTracingHitGroup
{
};

template <typename TShader> class TGlobalShader
{
  public:
	using ShaderType = TShader;

	static constexpr std::string_view GetShaderName() noexcept { return TShader::kShaderName; }
	static constexpr std::string_view GetShaderPackageName() noexcept { return TShader::kShaderPackageName; }
	static constexpr std::string_view GetBindingLayoutId() noexcept { return TShader::kBindingLayoutId; }
	static ShaderParameterStructDescriptor GetParameterStructDescriptor()
	{
		return TShader::FParameters::GetShaderParameterStructDescriptor();
	}
	static ShaderPermutationDomainDescriptor GetPermutationDomainDescriptor()
	{
		if constexpr (requires { typename TShader::FPermutationDomain; })
		{
			return TShader::FPermutationDomain::GetDescriptor();
		}
		else
		{
			return {};
		}
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

template <typename TShader> class TGlobalShaderAutoRegister final
{
  public:
	TGlobalShaderAutoRegister(
	    std::string_view sourcePath,
	    std::string_view entryPoint,
	    ShaderStage stage,
	    CookedShaderPackageFeatureFlags packageFeatures = TGlobalShader<TShader>::GetPackageFeatures())
	{
		GlobalShaderRegistry::Register(ShaderRegistrationDesc{
		    .ShaderName = TShader::kShaderName,
		    .PackageName = TGlobalShader<TShader>::GetShaderPackageName(),
		    .BindingLayoutId = TGlobalShader<TShader>::GetBindingLayoutId(),
		    .SourcePath = sourcePath,
		    .EntryPoint = entryPoint,
		    .Stage = stage,
		    .PackageKind = GetDefaultCookedShaderPackageKind(stage),
		    .PackageFeatures = packageFeatures,
		    .BuildParameterStructDescriptor = &TGlobalShader<TShader>::GetParameterStructDescriptor,
		    .BuildPermutationDomainDescriptor = &TGlobalShader<TShader>::GetPermutationDomainDescriptor,
		});
	}
};

template <typename TShader> class TRayTracingShaderAutoRegister final
{
  public:
	TRayTracingShaderAutoRegister(
	    std::string_view sourcePath,
	    std::string_view entryPoint)
	{
		GlobalShaderRegistry::Register(ShaderRegistrationDesc{
		    .ShaderName = TShader::kShaderName,
		    .PackageName = TGlobalShader<TShader>::GetShaderPackageName(),
		    .BindingLayoutId = TGlobalShader<TShader>::GetBindingLayoutId(),
		    .SourcePath = sourcePath,
		    .EntryPoint = entryPoint,
		    .Stage = ShaderStage::Count,
		    .PackageKind = CookedShaderPackageKind::RayTracingLibrary,
		    .PackageFeatures = TGlobalShader<TShader>::GetPackageFeatures() | CookedShaderPackageFeatureFlags::UsesAccelerationStructure,
		    .RayTracingExportKind = TShader::kRayTracingExportKind,
		    .RayTracingExportName = entryPoint,
		    .RayTracingPayloadSizeInBytes = TShader::kRayTracingPayloadSizeInBytes,
		    .RayTracingAttributeSizeInBytes = TShader::kRayTracingAttributeSizeInBytes,
		    .RayTracingMaxRecursionDepth = TShader::kRayTracingMaxRecursionDepth,
		    .BuildParameterStructDescriptor = &TGlobalShader<TShader>::GetParameterStructDescriptor,
		    .BuildPermutationDomainDescriptor = &TGlobalShader<TShader>::GetPermutationDomainDescriptor,
		});
	}
};

template <typename THitGroup> class TRayTracingHitGroupAutoRegister final
{
  public:
	TRayTracingHitGroupAutoRegister()
	{
		static_assert(requires { typename THitGroup::ClosestHit; }, "Ray tracing hit groups require a ClosestHit shader type.");
		using ClosestHit = typename THitGroup::ClosestHit;

		std::string_view anyHitShaderName;
		if constexpr (requires { typename THitGroup::AnyHit; })
		{
			using AnyHit = typename THitGroup::AnyHit;
			anyHitShaderName = AnyHit::kShaderName;
		}

		std::string_view intersectionShaderName;
		if constexpr (requires { typename THitGroup::Intersection; })
		{
			using Intersection = typename THitGroup::Intersection;
			intersectionShaderName = Intersection::kShaderName;
		}

		GlobalShaderRegistry::RegisterRayTracingHitGroup(RayTracingHitGroupRegistrationDesc{
		    .PackageName = THitGroup::kShaderPackageName,
		    .HitGroupName = THitGroup::kHitGroupName,
		    .ClosestHitShaderName = ClosestHit::kShaderName,
		    .AnyHitShaderName = anyHitShaderName,
		    .IntersectionShaderName = intersectionShaderName});
	}
};

template <typename TShader> class TShaderRef final
{
  public:
	TShaderRef() = default;

	static TShaderRef Get(ShaderPermutationKey permutationKey = 0) noexcept
	{
		TShaderRef ref;
		ref.m_shaderName = TShader::kShaderName;
		ref.m_permutationKey = permutationKey;
		return ref;
	}

	std::string_view GetShaderName() const noexcept { return m_shaderName; }
	ShaderPermutationKey GetPermutationKey() const noexcept { return m_permutationKey; }
	explicit operator bool() const noexcept { return !m_shaderName.empty(); }

  private:
	std::string_view m_shaderName;
	ShaderPermutationKey m_permutationKey = 0;
};

#define IMPLEMENT_GLOBAL_SHADER(Class, Path, Entry, StageName) \
	inline static const ::TGlobalShaderAutoRegister<Class> AutoRegisterGlobalShader_##Class{ \
	    Path, \
	    Entry, \
	    ::ShaderStage::StageName}

#define IMPLEMENT_RAY_TRACING_SHADER(Class, Path, Entry) \
	inline static const ::TRayTracingShaderAutoRegister<Class> AutoRegisterRayTracingShader_##Class{ \
	    Path, \
	    Entry}

#define IMPLEMENT_RAY_TRACING_HIT_GROUP(Class) \
	inline static const ::TRayTracingHitGroupAutoRegister<Class> AutoRegisterRayTracingHitGroup_##Class{}