#pragma once

#include "RendererAPI.h"
#include "ShaderParameters/PassParameterSet.h"

#include <cstdint>
#include <type_traits>
#include <utility>

class CommandContext;
class FrameGraph;
class PassBuilder;
class PassParameterLayout;
class PassBindingOverrides;
class RenderHardwareInterface;
class RenderBindingLayout;
class RenderPipelineState;
struct RenderGraphPassContext;
struct FrameContext;
struct RenderPassContext;

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

SPARKLE_RENDERER_API void DeclareShaderPassParameterUsages(PassBuilder& builder, const PassParameterSet& parameterSet) noexcept;
SPARKLE_RENDERER_API void DispatchComputeShaderPass(CommandContext& cmd, const ComputeDispatchDesc& dispatch) noexcept;
SPARKLE_RENDERER_API bool ValidateShaderPassLayout(
    const PassParameterLayout& layout,
    ShaderPassKind passKind,
    const char* passName) noexcept;
SPARKLE_RENDERER_API void BindComputeShaderPass(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    RenderHardwareInterface* renderHardwareInterface,
    const RenderBindingLayout& bindingLayout,
    const RenderPipelineState& pipelineState,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr) noexcept;

SPARKLE_RENDERER_API void BindRasterShaderPass(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    RenderHardwareInterface* renderHardwareInterface,
    const RenderBindingLayout& bindingLayout,
    const RenderPipelineState& pipelineState,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr) noexcept;

SPARKLE_RENDERER_API void ReportInvalidShaderPassParameterSet(const char* passName, const PassParameterSet& parameterSet) noexcept;

class SPARKLE_RENDERER_API ShaderPass
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
	static bool SetupParameterUsages(PassBuilder& builder, const TParameterBindings& parameters, const char* passName) noexcept
	{
		const PassParameterSet& parameterSet = GetPassParameterSet(parameters);
		if (!ValidateSetupParameterSet(parameterSet, passName))
		{
			return false;
		}

		DeclareShaderPassParameterUsages(builder, parameterSet);
		return true;
	}

	template <typename TParameterBindings>
	static bool ValidateExecutionParameters(
	    const TParameterBindings& parameters,
	    const char* passName,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0) noexcept
	{
		return ValidateParameterSet(GetPassParameterSet(parameters), passName, bindingNames, bindingNameCount);
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
			if (!IsFrameGraphParameter(parameters[index]))
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
	    std::uint32_t bindingNameCount = 0) noexcept
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
				if (binding != nullptr && !binding->IsBound())
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

	static bool IsFrameGraphParameter(const PassParameterDesc& parameter) noexcept
	{
		switch (parameter.Kind)
		{
			case ShaderParameterSemanticKind::ReadTexture:
			case ShaderParameterSemanticKind::ReadBuffer:
			case ShaderParameterSemanticKind::RWTexture:
			case ShaderParameterSemanticKind::RWBuffer:
			case ShaderParameterSemanticKind::RenderTarget:
			case ShaderParameterSemanticKind::DepthTarget:
				return true;
			case ShaderParameterSemanticKind::UniformData:
			case ShaderParameterSemanticKind::SamplerSet:
			case ShaderParameterSemanticKind::AccelerationStructure:
				return false;
			default:
				return false;
		}
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
	static bool Setup(PassBuilder& builder, const TParameterBindings& parameters, const char* passName = nullptr) noexcept
	{
		return SetupParameterUsages(builder, parameters, passName);
	}

	template <typename TParameterBindings>
	static bool Dispatch(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RenderBindingLayout& bindingLayout,
	    const RenderPipelineState& pipelineState,
	    const TParameterBindings& parameters,
	    const ComputeDispatchDesc& dispatch,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr) noexcept
	{
		if (!ValidateExecutionParameters(parameters, passName, bindingNames, bindingNameCount))
		{
			return false;
		}

		BindComputeShaderPass(
		    cmd,
		    frameGraph,
		    &renderHardwareInterface,
		    bindingLayout,
		    pipelineState,
		    GetPassParameterSet(parameters),
		    bindingNames,
		    bindingNameCount,
		    overrides);
		DispatchComputeShaderPass(cmd, dispatch);
		return true;
	}

	void Dispatch(CommandContext& cmd, const ComputeDispatchDesc& dispatch) const noexcept { DispatchComputeShaderPass(cmd, dispatch); }
};

template <typename TParameters> class RasterShaderPass : public ShaderPass
{
  public:
	using Parameters = TParameters;

	ShaderPassKind GetPassKind() const noexcept final { return ShaderPassKind::Raster; }

	template <typename TParameterBindings>
	static bool Setup(PassBuilder& builder, const TParameterBindings& parameters, const char* passName = nullptr) noexcept
	{
		return SetupParameterUsages(builder, parameters, passName);
	}

	template <typename TParameterBindings>
	static bool Bind(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface* renderHardwareInterface,
	    const RenderBindingLayout& bindingLayout,
	    const RenderPipelineState& pipelineState,
	    const TParameterBindings& parameters,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr) noexcept
	{
		if (!ValidateExecutionParameters(parameters, passName, bindingNames, bindingNameCount))
		{
			return false;
		}

		BindRasterShaderPass(
		    cmd,
		    frameGraph,
		    renderHardwareInterface,
		    bindingLayout,
		    pipelineState,
		    GetPassParameterSet(parameters),
		    bindingNames,
		    bindingNameCount,
		    overrides);
		return true;
	}

	virtual void Draw(RenderGraphPassContext& context, const Parameters& parameters) = 0;
};