#pragma once

#include "../../RHIAPI.h"
#include "../CookedShaderPackage.h"
#include "../ShaderStage.h"
#include "ShaderParameterStruct.h"

#include <cstdint>
#include <span>
#include <string>
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
};

struct RayTracingHitGroupRegistrationDesc final
{
	std::string_view PackageName;
	std::string_view HitGroupName;
	std::string_view ClosestHitExportName;
	std::string_view AnyHitExportName;
	std::string_view IntersectionExportName;
};

inline std::string BuildShaderPackageIdFromSourcePath(std::string_view sourcePath)
{
	const std::size_t slash = sourcePath.find_last_of("/\\");
	const std::size_t nameStart = slash == std::string_view::npos ? 0 : slash + 1;
	const std::size_t dot = sourcePath.find_last_of('.');
	if (dot != std::string_view::npos && dot > nameStart)
	{
		return std::string(sourcePath.substr(nameStart, dot - nameStart));
	}
	return nameStart < sourcePath.size() ? std::string(sourcePath.substr(nameStart)) : std::string{};
}

inline std::string GetShaderRegistrationPackageId(const ShaderRegistrationDesc& shader)
{
	if (!shader.PackageName.empty())
	{
		return std::string(shader.PackageName);
	}

	std::string packageId = BuildShaderPackageIdFromSourcePath(shader.SourcePath);
	return packageId.empty() ? std::string(shader.ShaderName) : packageId;
}

inline std::string GetShaderRegistrationBindingLayoutId(const ShaderRegistrationDesc& shader)
{
	return shader.BindingLayoutId.empty() ? GetShaderRegistrationPackageId(shader) : std::string(shader.BindingLayoutId);
}

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

template <typename TShader> struct TShaderSourceMetadata
{
	static constexpr std::string_view kSourcePath = "";
	static constexpr std::string_view kEntryPoint = "";
	static constexpr ShaderStage kStage = ShaderStage::Count;
};

template <typename TShader> class TGlobalShader
{
  public:
	using ShaderType = TShader;

	static constexpr std::string_view GetShaderName(std::string_view fallbackName) noexcept
	{
		if constexpr (requires { TShader::kShaderName; })
		{
			return TShader::kShaderName;
		}
		else
		{
			return fallbackName;
		}
	}
	static constexpr std::string_view GetShaderPackageName() noexcept
	{
		if constexpr (requires { TShader::kShaderPackageName; })
		{
			return TShader::kShaderPackageName;
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
		else
		{
			return {};
		}
	}
	static ShaderParameterStructDescriptor GetParameterStructDescriptor()
	{
		return TShader::FParameters::GetShaderParameterStructDescriptor();
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
	    std::string_view shaderName,
	    std::string_view sourcePath,
	    std::string_view entryPoint,
	    ShaderStage stage,
	    CookedShaderPackageFeatureFlags packageFeatures = TGlobalShader<TShader>::GetPackageFeatures())
	{
		GlobalShaderRegistry::Register(
		    ShaderRegistrationDesc{
		        .ShaderName = TGlobalShader<TShader>::GetShaderName(shaderName),
		        .PackageName = TGlobalShader<TShader>::GetShaderPackageName(),
		        .BindingLayoutId = TGlobalShader<TShader>::GetBindingLayoutId(),
		        .SourcePath = sourcePath,
		        .EntryPoint = entryPoint,
		        .Stage = stage,
		        .PackageKind = GetDefaultCookedShaderPackageKind(stage),
		        .PackageFeatures = packageFeatures,
		        .BuildParameterStructDescriptor = &TGlobalShader<TShader>::GetParameterStructDescriptor,
		    });
	}
};

template <typename TShader> class TRayTracingShaderAutoRegister final
{
  public:
	TRayTracingShaderAutoRegister(std::string_view shaderName, std::string_view sourcePath, std::string_view entryPoint)
	{
		GlobalShaderRegistry::Register(
		    ShaderRegistrationDesc{
		        .ShaderName = TGlobalShader<TShader>::GetShaderName(shaderName),
		        .PackageName = TGlobalShader<TShader>::GetShaderPackageName(),
		        .BindingLayoutId = TGlobalShader<TShader>::GetBindingLayoutId(),
		        .SourcePath = sourcePath,
		        .EntryPoint = entryPoint,
		        .Stage = ShaderStage::Count,
		        .PackageKind = CookedShaderPackageKind::RayTracingLibrary,
		        .PackageFeatures =
		            TGlobalShader<TShader>::GetPackageFeatures() | CookedShaderPackageFeatureFlags::UsesAccelerationStructure,
		        .RayTracingExportKind = TShader::kRayTracingExportKind,
		        .RayTracingExportName = entryPoint,
		        .RayTracingPayloadSizeInBytes = TShader::kRayTracingPayloadSizeInBytes,
		        .RayTracingAttributeSizeInBytes = TShader::kRayTracingAttributeSizeInBytes,
		        .RayTracingMaxRecursionDepth = TShader::kRayTracingMaxRecursionDepth,
		        .BuildParameterStructDescriptor = &TGlobalShader<TShader>::GetParameterStructDescriptor,
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
		constexpr std::string_view closestHitExportName = TShaderSourceMetadata<ClosestHit>::kEntryPoint;
		static_assert(
		    !closestHitExportName.empty(),
		    "Ray tracing hit group closest-hit shader must be registered with IMPLEMENT_RAY_TRACING_SHADER before the hit group.");

		std::string_view anyHitExportName;
		if constexpr (requires { typename THitGroup::AnyHit; })
		{
			using AnyHit = typename THitGroup::AnyHit;
			anyHitExportName = TShaderSourceMetadata<AnyHit>::kEntryPoint;
		}

		std::string_view intersectionExportName;
		if constexpr (requires { typename THitGroup::Intersection; })
		{
			using Intersection = typename THitGroup::Intersection;
			intersectionExportName = TShaderSourceMetadata<Intersection>::kEntryPoint;
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

template <typename TShader> class TShaderRef final
{
  public:
	TShaderRef() = default;

	static TShaderRef Get() noexcept
	{
		TShaderRef ref;
		ref.m_sourcePath = TShaderSourceMetadata<TShader>::kSourcePath;
		ref.m_entryPoint = TShaderSourceMetadata<TShader>::kEntryPoint;
		ref.m_stage = TShaderSourceMetadata<TShader>::kStage;
		return ref;
	}

	std::string_view GetSourcePath() const noexcept { return m_sourcePath; }
	std::string_view GetEntryPoint() const noexcept { return m_entryPoint; }
	ShaderStage GetStage() const noexcept { return m_stage; }
	std::string GetPackageId() const
	{
		if constexpr (requires { TShader::kShaderPackageName; })
		{
			return std::string(TShader::kShaderPackageName);
		}
		else
		{
			return BuildShaderPackageIdFromSourcePath(m_sourcePath);
		}
	}
	explicit operator bool() const noexcept { return !m_sourcePath.empty() && !m_entryPoint.empty(); }

  private:
	std::string_view m_sourcePath;
	std::string_view m_entryPoint;
	ShaderStage m_stage = ShaderStage::Count;
};

#define IMPLEMENT_GLOBAL_SHADER(Class, Path, Entry, StageName)                              \
	template <> struct TShaderSourceMetadata<Class>                                         \
	{                                                                                       \
		static constexpr std::string_view kSourcePath = Path;                               \
		static constexpr std::string_view kEntryPoint = Entry;                              \
		static constexpr ::ShaderStage kStage = ::ShaderStage::StageName;                   \
	};                                                                                      \
	inline static const ::TGlobalShaderAutoRegister<Class> AutoRegisterGlobalShader_##Class \
	{                                                                                       \
		#Class, Path, Entry, ::ShaderStage::StageName                                       \
	}

#define IMPLEMENT_RAY_TRACING_SHADER(Class, Path, Entry)                                            \
	template <> struct TShaderSourceMetadata<Class>                                                 \
	{                                                                                               \
		static constexpr std::string_view kSourcePath = Path;                                       \
		static constexpr std::string_view kEntryPoint = Entry;                                      \
		static constexpr ::ShaderStage kStage = ::ShaderStage::Count;                               \
	};                                                                                              \
	inline static const ::TRayTracingShaderAutoRegister<Class> AutoRegisterRayTracingShader_##Class \
	{                                                                                               \
		#Class, Path, Entry                                                                         \
	}

#define IMPLEMENT_RAY_TRACING_HIT_GROUP(Class) \
	inline static const ::TRayTracingHitGroupAutoRegister<Class> AutoRegisterRayTracingHitGroup_##Class {}