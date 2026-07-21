#include "PCH.h"

#include "CookedShaderBindingValidation.h"

#include "CookedShaderBindingDiagnostics.h"
#include "CookedShaderBindingRules.h"

#include <format>
#include <utility>

namespace
{
	class ReflectedBindingValidator final
	{
	  public:
		ReflectedBindingValidator(
		    const LoadedShaderPackage& package,
		    const ShaderPackageDefinition& definition,
		    const std::vector<PassParameterDesc>& expectedParameters,
		    CookedShaderBinaryFormat requiredBinaryFormat,
		    std::string& errorMessage) noexcept
		    : m_package(package),
		      m_definition(definition),
		      m_expectedParameters(expectedParameters),
		      m_requiredBinaryFormat(requiredBinaryFormat),
		      m_errorMessage(errorMessage)
		{
		}

		bool Validate()
		{
			if (!ValidateReflectionAvailability())
			{
				return false;
			}
			if (m_package.GetReflectionRecords().empty())
			{
				return true;
			}
			if (m_package.GetReflectionRecords().size() != m_package.GetBinaryRecords().size())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: reflection count {} does not match binary count {}",
				    m_definition.PackageId,
				    m_package.GetReflectionRecords().size(),
				    m_package.GetBinaryRecords().size()));
			}

			return ValidateReflectedResources() && ValidateExpectedParameters();
		}

	  private:
		bool Reject(std::string message)
		{
			m_errorMessage = std::move(message);
			return false;
		}

		bool ValidateReflectionAvailability()
		{
			if (!m_package.GetReflectionRecords().empty())
			{
				return true;
			}

			ShaderStageMask runtimeBindingStages = ShaderStageMask::None;
			for (const PassParameterDesc& expectedParameter : m_expectedParameters)
			{
				runtimeBindingStages |= CookedShaderBindingRules::ToPackageStageMask(expectedParameter.Visibility) &
				                        m_definition.ExpectedStages;
			}
			if (runtimeBindingStages == ShaderStageMask::None)
			{
				return true;
			}

			return Reject(std::format(
			    "Cooked shader package '{}' is missing {}/{} reflection required by runtime bindings for shader stages '{}'. Recook shaders.",
			    m_definition.PackageId,
			    CookedShaderBinaryFormatToString(m_requiredBinaryFormat),
			    GetRuntimeShaderCodegenTarget(m_requiredBinaryFormat),
			    FormatShaderStageMask(runtimeBindingStages)));
		}

		bool IsRelevantBinary(const CookedShaderBinaryRecord& binaryRecord) const noexcept
		{
			return m_package.IsRuntimeBinary(binaryRecord, m_requiredBinaryFormat) &&
			       CookedShaderBindingRules::HasAllStages(
			           m_definition.ExpectedStages, ToShaderStageMask(binaryRecord.Stage));
		}

		bool ResolveResourceBinding(
		    std::uint32_t bindingIndex, const CookedShaderResourceBindingRecord*& outResourceBinding)
		{
			if (bindingIndex >= m_package.GetResourceBindings().size())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: reflection resource binding index {} is out of range",
				    m_definition.PackageId,
				    bindingIndex));
			}
			outResourceBinding = &m_package.GetResourceBindings()[bindingIndex];
			return true;
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

		bool ValidateReflectedResources()
		{
			const auto& binaryRecords = m_package.GetBinaryRecords();
			const auto& reflectionRecords = m_package.GetReflectionRecords();
			for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size(); ++reflectionIndex)
			{
				if (IsRelevantBinary(binaryRecords[reflectionIndex]) &&
				    !ValidateReflectionResources(binaryRecords[reflectionIndex], reflectionRecords[reflectionIndex]))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateReflectionResources(
		    const CookedShaderBinaryRecord& binaryRecord, const CookedShaderReflectionRecord& reflection)
		{
			for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
			{
				const std::uint32_t bindingIndex = reflection.ResourceBindingOffset + resourceIndex;
				const CookedShaderResourceBindingRecord* resourceBinding = nullptr;
				if (!ResolveResourceBinding(bindingIndex, resourceBinding))
				{
					return false;
				}

				const std::string_view resourceName = m_package.ResolveString(
				    CookedShaderStringRef{resourceBinding->NameOffsetInBytes, resourceBinding->NameSizeInBytes});
				if (resourceName.empty())
				{
					return Reject(std::format(
					    "Cooked shader package '{}' failed package contract check: reflection resource {} has an invalid name string",
					    m_definition.PackageId,
					    bindingIndex));
				}
				if (!HasMatchingExpectedParameter(resourceName, resourceBinding->Kind, binaryRecord.Stage))
				{
					return Reject(CookedShaderBindingDiagnostics::Append(
					    std::format(
					        "Cooked shader package '{}' failed package contract check: stage {} reflects unexpected {} '{}' for target {}/{}. "
					        "Recook shaders to refresh stale package metadata.",
					        m_definition.PackageId,
					        static_cast<std::uint32_t>(binaryRecord.Stage),
					        CookedShaderBindingDiagnostics::FormatResourceKind(resourceBinding->Kind),
					        resourceName,
					        CookedShaderBinaryFormatToString(m_requiredBinaryFormat),
					        GetRuntimeShaderCodegenTarget(m_requiredBinaryFormat)),
					    m_package,
					    m_definition,
					    m_expectedParameters,
					    m_requiredBinaryFormat));
				}
			}
			return true;
		}

		bool ValidateExpectedParameters()
		{
			for (const PassParameterDesc& expectedParameter : m_expectedParameters)
			{
				bool foundMatch = false;
				if (!FindExpectedParameter(expectedParameter, foundMatch))
				{
					return false;
				}
				if (!foundMatch)
				{
					return Reject(CookedShaderBindingDiagnostics::Append(
					    std::format(
					        "Cooked shader package '{}' failed package contract check: runtime parameter '{}' (shader='{}') is missing reflected "
					        "target {}/{} bindings. Recook shaders to refresh stale package metadata.",
					        m_definition.PackageId,
					        expectedParameter.Name,
					        expectedParameter.GetShaderName(),
					        CookedShaderBinaryFormatToString(m_requiredBinaryFormat),
					        GetRuntimeShaderCodegenTarget(m_requiredBinaryFormat)),
					    m_package,
					    m_definition,
					    m_expectedParameters,
					    m_requiredBinaryFormat));
				}
			}
			return true;
		}

		bool FindExpectedParameter(const PassParameterDesc& expectedParameter, bool& outFoundMatch)
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
					const CookedShaderResourceBindingRecord* resourceBinding = nullptr;
					if (!ResolveResourceBinding(reflection.ResourceBindingOffset + resourceIndex, resourceBinding))
					{
						return false;
					}

					const std::string_view resourceName = m_package.ResolveString(
					    CookedShaderStringRef{resourceBinding->NameOffsetInBytes, resourceBinding->NameSizeInBytes});
					if (resourceName == expectedParameter.GetShaderName() &&
					    CookedShaderBindingRules::ResourceKindMatchesSemantic(resourceBinding->Kind, expectedParameter.Kind))
					{
						outFoundMatch = true;
						return true;
					}
				}
			}
			return true;
		}

		const LoadedShaderPackage& m_package;
		const ShaderPackageDefinition& m_definition;
		const std::vector<PassParameterDesc>& m_expectedParameters;
		CookedShaderBinaryFormat m_requiredBinaryFormat;
		std::string& m_errorMessage;
	};
}

bool CookedShaderBindingValidation::Validate(
    const LoadedShaderPackage& package,
    const ShaderPackageDefinition& definition,
    const std::vector<PassParameterDesc>& expectedParameters,
    CookedShaderBinaryFormat requiredBinaryFormat,
    std::string& outErrorMessage)
{
	return ReflectedBindingValidator(package, definition, expectedParameters, requiredBinaryFormat, outErrorMessage).Validate();
}
