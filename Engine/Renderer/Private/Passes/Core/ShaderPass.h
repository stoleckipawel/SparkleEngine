#pragma once

#include "Pipeline/PassBindingOverrides.h"
#include "ShaderParameters/PassParameterSet.h"

#include <cstdint>
#include <type_traits>
#include <utility>

class RenderCommandContext;
class FrameGraphResourceCommands;
class PassResourceBuilder;
class PassParameterLayout;
class RenderHardwareInterface;
class RenderBindingLayout;
class RenderPipeline;
struct PassExecutionContext;
struct FrameContext;
struct PassRuntimeContext;

enum class ShaderPassKind : std::uint8_t
{
	Compute,
	Raster,
};

struct ComputeDispatchDesc
{
	std::uint32_t GroupCountX = 1;
	std::uint32_t GroupCountY = 1;
	std::uint32_t GroupCountZ = 1;
};

bool DeclareShaderPassParameterUsages(
    PassResourceBuilder& builder,
    const PassParameterSet& parameterSet,
    const char* passName = nullptr) noexcept;
void DispatchComputeShaderPass(RenderCommandContext& commandContext, const ComputeDispatchDesc& dispatch) noexcept;
bool ValidateShaderPassLayout(
    const PassParameterLayout& layout,
    ShaderPassKind passKind,
    const char* passName) noexcept;
void BindComputeShaderPass(
    RenderCommandContext& commandContext,
	const FrameGraphResourceCommands& resources,
    RenderHardwareInterface* renderHardwareInterface,
    const RenderBindingLayout& bindingLayout,
    const RenderPipeline& pipeline,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr,
    bool bindLayout = true) noexcept;

void BindRasterShaderPass(
    RenderCommandContext& commandContext,
	const FrameGraphResourceCommands& resources,
    RenderHardwareInterface* renderHardwareInterface,
    const RenderBindingLayout& bindingLayout,
    const RenderPipeline& pipeline,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr,
    bool bindLayout = true) noexcept;

void ReportInvalidShaderPassParameterSet(const char* passName, const PassParameterSet& parameterSet) noexcept;

class ShaderPass
{
  public:
	virtual ~ShaderPass() noexcept = default;

	virtual const char* GetPassName() const noexcept = 0;
	virtual ShaderPassKind GetPassKind() const noexcept = 0;

  protected:
	template <typename T> struct AlwaysFalse : std::false_type
	{
	};

	template <typename T> struct RemoveCvRef
	{
		using Type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
	};

	template <typename T, typename = void> struct HasPassParameterSetAccessor : std::false_type
	{
	};

	template <typename T>
	struct HasPassParameterSetAccessor<T, std::void_t<decltype(std::declval<const T&>().GetPassParameterSet())>> : std::true_type
	{
	};

	template <typename TParameterBindings> static const PassParameterSet& GetPassParameterSet(const TParameterBindings& parameters) noexcept
	{
		return GetPassParameterSetImpl(
		    parameters,
		    typename std::is_same<typename RemoveCvRef<TParameterBindings>::Type, PassParameterSet>::type{});
	}

	template <typename TParameterBindings>
	static bool SetupParameterUsages(PassResourceBuilder& builder, const TParameterBindings& parameters, const char* passName) noexcept
	{
		const PassParameterSet& parameterSet = GetPassParameterSet(parameters);
		if (!ValidateSetupParameterSet(parameterSet, passName))
		{
			return false;
		}

		return DeclareShaderPassParameterUsages(builder, parameterSet, passName);
	}

	template <typename TParameterBindings>
	static bool ValidateExecutionParameters(
	    const TParameterBindings& parameters,
	    const char* passName,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr) noexcept
	{
		return ValidateParameterSet(GetPassParameterSet(parameters), passName, bindingNames, bindingNameCount, overrides);
	}

	static bool ValidateSetupParameterSet(const PassParameterSet& parameterSet, const char* passName) noexcept
	{
		if (!parameterSet.HasLayout())
		{
			ReportInvalidShaderPassParameterSet(passName, parameterSet);
			return false;
		}

		const PassParameterLayout* layout = parameterSet.GetLayout();
		if (layout == nullptr)
		{
			ReportInvalidShaderPassParameterSet(passName, parameterSet);
			return false;
		}

		const std::vector<PassParameterDesc>& parameters = layout->GetParameters();
		for (std::uint32_t index = 0; index < static_cast<std::uint32_t>(parameters.size()); ++index)
		{
			if (!parameterSet.UsesGraphResource(index))
			{
				continue;
			}

			const PassParameterBinding* binding = parameterSet.GetBinding(index);
			if (binding == nullptr || !binding->IsBound())
			{
				ReportInvalidShaderPassParameterSet(passName, parameterSet);
				return false;
			}
		}

		return true;
	}

	static bool ValidateParameterSet(
	    const PassParameterSet& parameterSet,
	    const char* passName,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr) noexcept
	{
		if (!parameterSet.HasLayout())
		{
			ReportInvalidShaderPassParameterSet(passName, parameterSet);
			return false;
		}

		if (bindingNames != nullptr && bindingNameCount > 0)
		{
			for (std::uint32_t index = 0; index < bindingNameCount; ++index)
			{
				const PassParameterBinding* binding = parameterSet.FindBinding(bindingNames[index]);
				if (binding != nullptr && !binding->IsBound() && !HasBindingOverride(overrides, bindingNames[index]))
				{
					ReportInvalidShaderPassParameterSet(passName, parameterSet);
					return false;
				}
			}

			return true;
		}

		if (!parameterSet.HasAllRequiredBindings())
		{
			ReportInvalidShaderPassParameterSet(passName, parameterSet);
			return false;
		}

		return true;
	}

	static bool HasBindingOverride(const PassBindingOverrides* overrides, const char* bindingName) noexcept
	{
		if (overrides == nullptr || bindingName == nullptr)
		{
			return false;
		}

		for (const PassBindingOverride& bindingOverride : overrides->GetOverrides())
		{
			if (bindingOverride.Name == bindingName)
			{
				return true;
			}
		}

		return false;
	}

  private:
	template <typename TParameterBindings>
	static const PassParameterSet& GetPassParameterSetImpl(const TParameterBindings& parameters, std::true_type) noexcept
	{
		return parameters;
	}

	template <typename TParameterBindings>
	static const PassParameterSet& GetPassParameterSetImpl(const TParameterBindings& parameters, std::false_type) noexcept
	{
		return GetPassParameterSetFromAccessor(parameters, typename HasPassParameterSetAccessor<TParameterBindings>::type{});
	}

	template <typename TParameterBindings>
	static const PassParameterSet& GetPassParameterSetFromAccessor(const TParameterBindings& parameters, std::true_type) noexcept
	{
		return parameters.GetPassParameterSet();
	}

	template <typename TParameterBindings>
	static const PassParameterSet& GetPassParameterSetFromAccessor(const TParameterBindings&, std::false_type) noexcept
	{
		static_assert(
		    AlwaysFalse<TParameterBindings>::value,
		    "Shader pass parameters must expose GetPassParameterSet() or be PassParameterSet.");
#if defined(_MSC_VER)
		__assume(false);
#else
		__builtin_unreachable();
#endif
	}
};

template <typename TParameters> class ComputeShaderPass : public ShaderPass
{
  public:
	using Parameters = TParameters;

	ShaderPassKind GetPassKind() const noexcept final { return ShaderPassKind::Compute; }

	template <typename TParameterBindings>
	static bool Setup(PassResourceBuilder& builder, const TParameterBindings& parameters, const char* passName = nullptr) noexcept
	{
		return SetupParameterUsages(builder, parameters, passName);
	}

	template <typename TParameterBindings>
	static bool Dispatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RenderBindingLayout& bindingLayout,
	    const RenderPipeline& pipeline,
	    const TParameterBindings& parameters,
	    const ComputeDispatchDesc& dispatch,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr,
	    bool bindLayout = true) noexcept
	{
		if (!ValidateExecutionParameters(parameters, passName, bindingNames, bindingNameCount, overrides))
		{
			return false;
		}

		BindComputeShaderPass(
		    commandContext,
		    resources,
		    &renderHardwareInterface,
		    bindingLayout,
		    pipeline,
		    GetPassParameterSet(parameters),
		    bindingNames,
		    bindingNameCount,
		    overrides,
		    bindLayout);
		DispatchComputeShaderPass(commandContext, dispatch);
		return true;
	}

	void Dispatch(RenderCommandContext& commandContext, const ComputeDispatchDesc& dispatch) const noexcept { DispatchComputeShaderPass(commandContext, dispatch); }
};

template <typename TParameters> class RasterShaderPass : public ShaderPass
{
  public:
	using Parameters = TParameters;

	ShaderPassKind GetPassKind() const noexcept final { return ShaderPassKind::Raster; }

	template <typename TParameterBindings>
	static bool Setup(PassResourceBuilder& builder, const TParameterBindings& parameters, const char* passName = nullptr) noexcept
	{
		return SetupParameterUsages(builder, parameters, passName);
	}

	template <typename TParameterBindings>
	static bool Bind(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface* renderHardwareInterface,
	    const RenderBindingLayout& bindingLayout,
	    const RenderPipeline& pipeline,
	    const TParameterBindings& parameters,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr,
	    bool bindLayout = true) noexcept
	{
		if (!ValidateExecutionParameters(parameters, passName, bindingNames, bindingNameCount, overrides))
		{
			return false;
		}

		BindRasterShaderPass(
		    commandContext,
		    resources,
		    renderHardwareInterface,
		    bindingLayout,
		    pipeline,
		    GetPassParameterSet(parameters),
		    bindingNames,
		    bindingNameCount,
		    overrides,
		    bindLayout);
		return true;
	}

	virtual void Draw(PassExecutionContext& context, const Parameters& parameters) = 0;
};
