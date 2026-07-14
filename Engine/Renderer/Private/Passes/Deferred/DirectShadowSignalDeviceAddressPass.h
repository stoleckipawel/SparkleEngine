#pragma once

#include "Passes/Deferred/DirectShadowSignalPassCommon.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct DirectShadowSignalDeviceAddressPassParameters : DirectShadowSignalRayQueryPassParameters
{
	static void Describe(ShaderParameterStructBuilder<DirectShadowSignalDeviceAddressPassParameters>& builder)
	{
		DirectShadowSignalRayQueryPassParameters::Describe(builder);
	}
};

class DirectShadowSignalDeviceAddressPass final
{
  public:
	static constexpr const char* PassName = "DirectShadowSignalDeviceAddress";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectShadowSignalDeviceAddressPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectShadowSignalDeviceAddressPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};
