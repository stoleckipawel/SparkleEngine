#include "PCH.h"

#include "CookedShaderBindingValidation.h"

#include "CookedShaderBindingDiagnostics.h"
#include "CookedShaderBindingRules.h"
#include "Core/Public/Diagnostics/Error.h"

#include <format>
#include <utility>

class CookedShaderBindingValidationImplementation final
{
  public:
	class ReflectedBindingValidator final
	{
	  public:
		ReflectedBindingValidator(
		    const LoadedShaderPackage& package,
		    const ShaderPackageDefinition& definition,
		    const std::vector<PassParameterDesc>& expectedParameters,
		    CookedShaderBinaryFormat runtimeBinaryFormat) noexcept
		    : m_package(package),
		      m_definition(definition),
		      m_expectedParameters(expectedParameters),
		      m_runtimeBinaryFormat(runtimeBinaryFormat)
		{
		}

		void Validate()
		{
			ValidateReflectionAvailability();
			if (m_package.GetReflectionRecords().empty())
			{
				return;
			}
			if (m_package.GetReflectionRecords().size() != m_package.GetBinaryRecords().size())
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: reflection count {} does not match binary count {}",
				    m_definition.PackageId,
				    m_package.GetReflectionRecords().size(),
				    m_package.GetBinaryRecords().size()));
			}

			ValidateReflectedResources();
			ValidateExpectedParameters();
		}

	  private:
		[[noreturn]] static void Reject(std::string message)
		{
			throw Diagnostics::Error(std::move(message));
		}

		void ValidateReflectionAvailability()
		{
			if (!m_package.GetReflectionRecords().empty())
			{
				return;
			}

			ShaderStageMask runtimeBindingStages = ShaderStageMask::None;
			for (const PassParameterDesc& expectedParameter : m_expectedParameters)
			{
				runtimeBindingStages |= CookedShaderBindingRules::ToPackageStageMask(expectedParameter.Visibility) &
				                        m_definition.ExpectedStages;
			}
			if (runtimeBindingStages == ShaderStageMask::None)
			{
				return;
			}

			Reject(std::format(
			    "Cooked shader package '{}' is missing {}/{} reflection for runtime binding stages '{}'. Recook shaders.",
			    m_definition.PackageId,
			    CookedShaderBinaryFormatToString(m_runtimeBinaryFormat),
			    GetRuntimeShaderCodegenTarget(m_runtimeBinaryFormat),
			    FormatShaderStageMask(runtimeBindingStages)));
		}

		bool IsRelevantBinary(const CookedShaderBinaryRecord& binaryRecord) const noexcept
		{
			return m_package.IsRuntimeBinary(binaryRecord, m_runtimeBinaryFormat) &&
			       CookedShaderBindingRules::HasAllStages(
			           m_definition.ExpectedStages, ToShaderStageMask(binaryRecord.Stage));
		}

		const CookedShaderResourceBindingRecord& ResolveResourceBinding(std::uint32_t bindingIndex)
		{
			if (bindingIndex >= m_package.GetResourceBindings().size())
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: reflection resource binding index {} is out of range",
				    m_definition.PackageId,
				    bindingIndex));
			}
			return m_package.GetResourceBindings()[bindingIndex];
		}

		bool HasMatchingExpectedParameter(
		    std::string_view resourceName, CookedShaderResourceKind resourceKind, ShaderStage stage) const noexcept
		{
			const ShaderStageMask stageMask = ToShaderStageMask(stage);
			for (const PassParameterDesc& expectedParameter : m_expectedParameters)
			{
				if (CookedShaderBindingRules::HasAllStages(
				        CookedShaderBindingRules::ToPackageStageMask(expectedParameter.Visibility), stageMask) &&
				    expectedParameter.GetShaderName() == resourceName &&
				    CookedShaderBindingRules::ResourceKindMatchesSemantic(resourceKind, expectedParameter.Kind))
				{
					return true;
				}
			}
			return false;
		}

		void ValidateReflectedResources()
		{
			const auto& binaryRecords = m_package.GetBinaryRecords();
			const auto& reflectionRecords = m_package.GetReflectionRecords();
			for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size(); ++reflectionIndex)
			{
				if (IsRelevantBinary(binaryRecords[reflectionIndex]))
				{
					ValidateReflectionResources(binaryRecords[reflectionIndex], reflectionRecords[reflectionIndex]);
				}
			}
		}

		void ValidateReflectionResources(
		    const CookedShaderBinaryRecord& binaryRecord, const CookedShaderReflectionRecord& reflection)
		{
			for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
			{
				const std::uint32_t bindingIndex = reflection.ResourceBindingOffset + resourceIndex;
				const CookedShaderResourceBindingRecord& resourceBinding = ResolveResourceBinding(bindingIndex);

				const std::string_view resourceName = m_package.ResolveString(
				    CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
				if (resourceName.empty())
				{
					Reject(std::format(
					    "Cooked shader package '{}' failed package contract check: reflection resource {} has an invalid name string",
					    m_definition.PackageId,
					    bindingIndex));
				}
				if (!HasMatchingExpectedParameter(resourceName, resourceBinding.Kind, binaryRecord.Stage))
				{
					Reject(CookedShaderBindingDiagnostics::Append(
					    std::format(
					        "Cooked shader package '{}' failed package contract check: stage {} reflects unexpected {} '{}' for target {}/{}. "
					        "Recook shaders to refresh stale package metadata.",
					        m_definition.PackageId,
					        static_cast<std::uint32_t>(binaryRecord.Stage),
					        CookedShaderBindingDiagnostics::FormatResourceKind(resourceBinding.Kind),
					        resourceName,
					        CookedShaderBinaryFormatToString(m_runtimeBinaryFormat),
					        GetRuntimeShaderCodegenTarget(m_runtimeBinaryFormat)),
					    m_package,
					    m_definition,
					    m_expectedParameters,
					    m_runtimeBinaryFormat));
				}
			}
		}

		void ValidateExpectedParameters()
		{
			for (const PassParameterDesc& expectedParameter : m_expectedParameters)
			{
				if (!ContainsExpectedParameter(expectedParameter))
				{
					Reject(CookedShaderBindingDiagnostics::Append(
					    std::format(
					        "Cooked shader package '{}' failed package contract check: runtime parameter '{}' (shader='{}') is missing reflected "
					        "target {}/{} bindings. Recook shaders to refresh stale package metadata.",
					        m_definition.PackageId,
					        expectedParameter.Name,
					        expectedParameter.GetShaderName(),
					        CookedShaderBinaryFormatToString(m_runtimeBinaryFormat),
					        GetRuntimeShaderCodegenTarget(m_runtimeBinaryFormat)),
					    m_package,
					    m_definition,
					    m_expectedParameters,
					    m_runtimeBinaryFormat));
				}
			}
		}

		bool ContainsExpectedParameter(const PassParameterDesc& expectedParameter)
		{
			const auto& binaryRecords = m_package.GetBinaryRecords();
			const auto& reflectionRecords = m_package.GetReflectionRecords();
			for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size(); ++reflectionIndex)
			{
				const CookedShaderBinaryRecord& binaryRecord = binaryRecords[reflectionIndex];
				const ShaderStageMask stageMask = ToShaderStageMask(binaryRecord.Stage);
				if (!IsRelevantBinary(binaryRecord) ||
				    !CookedShaderBindingRules::HasAllStages(
				        CookedShaderBindingRules::ToPackageStageMask(expectedParameter.Visibility), stageMask))
				{
					continue;
				}

				const CookedShaderReflectionRecord& reflection = reflectionRecords[reflectionIndex];
				for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
				{
					const CookedShaderResourceBindingRecord& resourceBinding =
					    ResolveResourceBinding(reflection.ResourceBindingOffset + resourceIndex);

					const std::string_view resourceName = m_package.ResolveString(
					    CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
					if (resourceName == expectedParameter.GetShaderName() &&
					    CookedShaderBindingRules::ResourceKindMatchesSemantic(resourceBinding.Kind, expectedParameter.Kind))
					{
						return true;
					}
				}
			}
			return false;
		}

		const LoadedShaderPackage& m_package;
		const ShaderPackageDefinition& m_definition;
		const std::vector<PassParameterDesc>& m_expectedParameters;
		CookedShaderBinaryFormat m_runtimeBinaryFormat;
	};
};

void CookedShaderBindingValidation::Validate(
    const LoadedShaderPackage& package,
    const ShaderPackageDefinition& definition,
    const std::vector<PassParameterDesc>& expectedParameters,
    CookedShaderBinaryFormat runtimeBinaryFormat)
{
	CookedShaderBindingValidationImplementation::ReflectedBindingValidator(
	    package, definition, expectedParameters, runtimeBinaryFormat)
	    .Validate();
}
