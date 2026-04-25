#pragma once

#include "../../RHIAPI.h"
#include "../../ShaderParameters/PassParameterLayout.h"
#include "../ShaderStage.h"
#include "ShaderParameterStruct.h"
#include "ShaderPermutation.h"

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
	ShaderParameterStructDescriptor (*BuildParameterStructDescriptor)() = nullptr;
	ShaderPermutationDomainDescriptor (*BuildPermutationDomainDescriptor)() = nullptr;
	PassParameterLayout (*BuildPackageBindingLayout)() = nullptr;
};

class SPARKLE_RHI_API GlobalShaderRegistry final
{
  public:
	GlobalShaderRegistry() = delete;

	static void Register(ShaderRegistrationDesc desc);
	static std::span<const ShaderRegistrationDesc> GetRegistrations() noexcept;
	static const ShaderRegistrationDesc* FindByName(std::string_view shaderName) noexcept;
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
	static PassParameterLayout BuildPackageBindingLayout()
	{
		if constexpr (requires { TShader::BuildPackageBindingLayout(); })
		{
			return TShader::BuildPackageBindingLayout();
		}
		else
		{
			const std::string debugName(TShader::kBindingLayoutId.empty() ? "Empty" : TShader::kBindingLayoutId);
			return PassParameterLayout(debugName.c_str());
		}
	}
};

template <typename TShader> class TGlobalShaderAutoRegister final
{
  public:
	TGlobalShaderAutoRegister(std::string_view sourcePath, std::string_view entryPoint, ShaderStage stage)
	{
		GlobalShaderRegistry::Register(ShaderRegistrationDesc{
		    .ShaderName = TShader::kShaderName,
		    .PackageName = TGlobalShader<TShader>::GetShaderPackageName(),
		    .BindingLayoutId = TGlobalShader<TShader>::GetBindingLayoutId(),
		    .SourcePath = sourcePath,
		    .EntryPoint = entryPoint,
		    .Stage = stage,
		    .BuildParameterStructDescriptor = &TGlobalShader<TShader>::GetParameterStructDescriptor,
		    .BuildPermutationDomainDescriptor = &TGlobalShader<TShader>::GetPermutationDomainDescriptor,
		    .BuildPackageBindingLayout = &TGlobalShader<TShader>::BuildPackageBindingLayout,
		});
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