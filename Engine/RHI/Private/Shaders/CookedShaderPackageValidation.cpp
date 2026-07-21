#include "PCH.h"

#include "Shaders/CookedShaderPackageCache.h"

#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageContract.h"
#include "Shaders/ShaderRayTracingMetadataValidation.h"

#include <array>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "CookedShaderBindingValidation.h"
#include "CookedShaderBindingRules.h"

namespace
{
	constexpr std::array<ShaderStage, 6> kKnownShaderStages =
	    {ShaderStage::Vertex, ShaderStage::Pixel, ShaderStage::Geometry, ShaderStage::Hull, ShaderStage::Domain, ShaderStage::Compute};

	using CookedShaderBindingRules::HasAllStages;
	using CookedShaderBindingRules::ToPackageStageMask;

	class CookedShaderPackageValidator final
	{
	  public:
		CookedShaderPackageValidator(
		    const LoadedShaderPackage& package,
		    const ShaderPackageDefinition& definition,
		    const PassParameterLayout& expectedBindingLayout,
		    CookedShaderBinaryFormat requiredBinaryFormat,
		    std::string& errorMessage) noexcept
		    : m_package(package),
		      m_definition(definition),
		      m_expectedBindingLayout(expectedBindingLayout),
		      m_expectedParameters(expectedBindingLayout.GetParameters()),
		      m_requiredBinaryFormat(requiredBinaryFormat),
		      m_requiredCodegenTarget(GetRuntimeShaderCodegenTarget(requiredBinaryFormat)),
		      m_expectedBindingLayoutHash(BuildPassParameterLayoutHash(expectedBindingLayout)),
		      m_errorMessage(errorMessage)
		{
		}

		bool Validate()
		{
			if (!ValidateHeader() || !ValidateRayTracingFeatures() || !ValidatePipelineLayouts() || !ValidateDeclaredStages() ||
			    !ValidateLogicalBindingRecords() ||
			    !CookedShaderBindingValidation::Validate(
			        m_package, m_definition, m_expectedParameters, m_requiredBinaryFormat, m_errorMessage) ||
			    !ValidateBinaries())
			{
				return false;
			}

			m_errorMessage.clear();
			return true;
		}

	  private:
		bool Reject(std::string message)
		{
			m_errorMessage = std::move(message);
			return false;
		}

		bool ValidateHeader()
		{
			if (!m_package.IsValid())
			{
				return Reject("Cooked shader package payload is invalid.");
			}

			const std::uint64_t expectedPackageKey = BuildShaderPackageKey(m_definition.PackageId);
			if (m_package.GetHeader().ShaderPackageKey != expectedPackageKey)
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: field=ShaderPackageKey expected={} actual={}",
				    m_definition.PackageId,
				    Formatting::FormatHexUInt64(expectedPackageKey),
				    Formatting::FormatHexUInt64(m_package.GetHeader().ShaderPackageKey)));
			}

			if (m_package.GetHeader().SourceIdentityHash == 0)
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: field=SourceIdentityHash actual=0",
				    m_definition.PackageId));
			}

			if (m_package.GetHeader().ShaderModelMajor != CookedShaderPackageContract::ShaderModelMajor ||
			    m_package.GetHeader().ShaderModelMinor != CookedShaderPackageContract::ShaderModelMinor)
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: field=ShaderModel expected={}.{} actual={}.{}",
				    m_definition.PackageId,
				    CookedShaderPackageContract::ShaderModelMajor,
				    CookedShaderPackageContract::ShaderModelMinor,
				    m_package.GetHeader().ShaderModelMajor,
				    m_package.GetHeader().ShaderModelMinor));
			}

			return true;
		}

		bool ValidateRayTracingFeatures()
		{
			if (m_package.GetHeader().PackageKind == CookedShaderPackageKind::RayTracingLibrary)
			{
				RhiRayTracingCapabilities metadataOnlyCapabilities{};
				metadataOnlyCapabilities.SupportsRayTracing = true;
				if (!m_package.ValidateRayTracingLibraryMetadata(metadataOnlyCapabilities, m_requiredBinaryFormat, m_errorMessage))
				{
					return false;
				}

				return Reject(std::format(
				    "Cooked shader package '{}' is a ray tracing library package with valid metadata, but runtime RT state object execution is not enabled yet.",
				    m_definition.PackageId));
			}

			if (HasCookedShaderPackageFeature(
			        m_package.GetHeader().PackageFeatures, CookedShaderPackageFeatureFlags::UsesInlineRayQuery) &&
			    !ShaderRayTracingMetadataValidation::ValidateInlineRayQueryMetadata(
			        m_package, m_requiredBinaryFormat, m_errorMessage))
			{
				const std::string inlineRayQueryError = m_errorMessage;
				return Reject(std::format(
				    "Cooked shader package '{}' failed inline ray query metadata validation: {}",
				    m_definition.PackageId,
				    inlineRayQueryError));
			}

			return true;
		}

		bool ValidatePipelineLayouts()
		{
			if (m_package.GetHeader().BindingLayoutHash != m_expectedBindingLayoutHash)
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: field=BindingLayoutHash bindingLayout='{}' expected={} actual={}",
				    m_definition.PackageId,
				    m_expectedBindingLayout.GetDebugName(),
				    Formatting::FormatHexUInt64(m_expectedBindingLayoutHash),
				    Formatting::FormatHexUInt64(m_package.GetHeader().BindingLayoutHash)));
			}

			if (m_package.GetPipelineLayoutRecords().empty())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: no cooked pipeline layout artifact records were found. Recook shaders.",
				    m_definition.PackageId));
			}

			std::uint32_t runtimeLayoutRecordCount = 0;
			for (const CookedShaderPipelineLayoutRecord& layoutRecord : m_package.GetPipelineLayoutRecords())
			{
				if (!ValidatePipelineLayoutRecord(layoutRecord, runtimeLayoutRecordCount))
				{
					return false;
				}
			}

			if (runtimeLayoutRecordCount != 1)
			{
				return Reject(std::format(
				    "Cooked shader package '{}' requires exactly one pipeline layout for runtime target '{}' but contains {}",
				    m_definition.PackageId,
				    m_requiredCodegenTarget,
				    runtimeLayoutRecordCount));
			}

			return true;
		}

		bool ValidatePipelineLayoutRecord(const CookedShaderPipelineLayoutRecord& layoutRecord, std::uint32_t& runtimeRecordCount)
		{
			if (layoutRecord.BindingLayoutHash != m_expectedBindingLayoutHash)
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: pipeline layout BindingLayoutHash expected={} actual={}",
				    m_definition.PackageId,
				    Formatting::FormatHexUInt64(m_expectedBindingLayoutHash),
				    Formatting::FormatHexUInt64(layoutRecord.BindingLayoutHash)));
			}

			if (layoutRecord.BindingRecordOffset + layoutRecord.BindingRecordCount > m_package.GetBindingRecords().size())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: pipeline layout binding range is out of bounds",
				    m_definition.PackageId));
			}

			const std::string_view codegenTarget = m_package.ResolveString(layoutRecord.CodegenTarget);
			if (codegenTarget.empty())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: pipeline layout CodegenTarget is empty",
				    m_definition.PackageId));
			}

			runtimeRecordCount += codegenTarget == m_requiredCodegenTarget ? 1u : 0u;
			return true;
		}

		bool ValidateDeclaredStages()
		{
			if (HasAllStages(m_package.GetHeader().DeclaredStages, m_definition.ExpectedStages))
			{
				return true;
			}

			return Reject(std::format(
			    "Cooked shader package '{}' failed package contract check: field=DeclaredStages expected='{}' actual='{}'",
			    m_definition.PackageId,
			    FormatShaderStageMask(m_definition.ExpectedStages),
			    FormatShaderStageMask(m_package.GetHeader().DeclaredStages)));
		}

		bool ValidateLogicalBindingRecords()
		{
			if (m_package.GetBindingRecords().size() != m_expectedParameters.size())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' declares {} binding records but runtime layout '{}' expects {}",
				    m_definition.PackageId,
				    m_package.GetBindingRecords().size(),
				    m_expectedBindingLayout.GetDebugName(),
				    m_expectedParameters.size()));
			}

			for (std::size_t parameterIndex = 0; parameterIndex < m_expectedParameters.size(); ++parameterIndex)
			{
				if (!ValidateLogicalBindingRecord(parameterIndex))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateLogicalBindingRecord(std::size_t parameterIndex)
		{
			const PassParameterDesc& expectedParameter = m_expectedParameters[parameterIndex];
			const CookedShaderBindingRecord& bindingRecord = m_package.GetBindingRecords()[parameterIndex];
			const std::string_view bindingName = m_package.ResolveString(bindingRecord.Name);
			if (bindingName.empty())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' has an invalid binding name string for binding index {}",
				    m_definition.PackageId,
				    parameterIndex));
			}

			const bool matches = bindingRecord.LogicalBindingIndex == parameterIndex && bindingName == expectedParameter.Name &&
			                     bindingRecord.SemanticKind == expectedParameter.Kind &&
			                     bindingRecord.ResourceDomain == expectedParameter.ResourceDomain &&
			                     bindingRecord.Access == expectedParameter.Access &&
			                     bindingRecord.VisibilityMask == ToPackageStageMask(expectedParameter.Visibility) &&
			                     bindingRecord.ArrayCount == expectedParameter.ArrayCount &&
			                     bindingRecord.ValueSizeInBytes == expectedParameter.ValueSizeInBytes;
			if (matches)
			{
				return true;
			}

			return Reject(std::format(
			    "Cooked shader package '{}' binding record {} does not match runtime layout parameter '{}'",
			    m_definition.PackageId,
			    parameterIndex,
			    expectedParameter.Name));
		}

		bool ValidateBinaries()
		{
			std::array<bool, static_cast<std::size_t>(ShaderStage::Count)> hasRequiredBinaryForStage = {};
			for (const CookedShaderBinaryRecord& binaryRecord : m_package.GetBinaryRecords())
			{
				if (!ValidateBinaryRecord(binaryRecord, hasRequiredBinaryForStage))
				{
					return false;
				}
			}

			return ValidateRequiredStages(hasRequiredBinaryForStage);
		}

		bool ValidateBinaryRecord(
		    const CookedShaderBinaryRecord& binaryRecord,
		    std::array<bool, static_cast<std::size_t>(ShaderStage::Count)>& hasRequiredBinaryForStage)
		{
			if (binaryRecord.Stage == ShaderStage::Count)
			{
				return Reject(std::format(
				    "Cooked shader package '{}' contains an invalid shader stage record", m_definition.PackageId));
			}
			if (m_package.ResolveString(binaryRecord.EntryPoint).empty())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' contains an invalid entry point string", m_definition.PackageId));
			}
			if (binaryRecord.DebugArtifact && m_package.ResolveString(binaryRecord.DebugArtifact).empty())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' contains an invalid debug artifact string", m_definition.PackageId));
			}
			if (m_package.ResolveString(binaryRecord.CodegenTarget).empty())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' contains a binary with an invalid CodegenTarget string", m_definition.PackageId));
			}

			const ShaderBytecode bytecode = m_package.GetBytecode(binaryRecord);
			if (!bytecode.IsValid())
			{
				return Reject(std::format(
				    "Cooked shader package '{}' contains an invalid bytecode blob for stage {}",
				    m_definition.PackageId,
				    static_cast<std::uint32_t>(binaryRecord.Stage)));
			}
			if (Hash::Fnv1a64(bytecode.Data, bytecode.Size) != binaryRecord.BytecodeHash)
			{
				return Reject(std::format(
				    "Cooked shader package '{}' failed bytecode hash validation for stage {}",
				    m_definition.PackageId,
				    static_cast<std::uint32_t>(binaryRecord.Stage)));
			}
			if (!HasAllStages(m_package.GetHeader().DeclaredStages, ToShaderStageMask(binaryRecord.Stage)))
			{
				return Reject(std::format(
				    "Cooked shader package '{}' contains a stage record outside its declared stage mask", m_definition.PackageId));
			}

			if (!m_package.IsRuntimeBinary(binaryRecord, m_requiredBinaryFormat))
			{
				return true;
			}

			const std::size_t stageIndex = static_cast<std::size_t>(binaryRecord.Stage);
			if (hasRequiredBinaryForStage[stageIndex])
			{
				return Reject(std::format(
				    "Cooked shader package '{}' contains more than one {}/{} binary for stage {}",
				    m_definition.PackageId,
				    CookedShaderBinaryFormatToString(m_requiredBinaryFormat),
				    m_requiredCodegenTarget,
				    stageIndex));
			}

			hasRequiredBinaryForStage[stageIndex] = true;
			return true;
		}

		bool ValidateRequiredStages(const std::array<bool, static_cast<std::size_t>(ShaderStage::Count)>& availableStages)
		{
			for (const ShaderStage stage : kKnownShaderStages)
			{
				if (!HasAllStages(m_definition.ExpectedStages, ToShaderStageMask(stage)) ||
				    availableStages[static_cast<std::size_t>(stage)])
				{
					continue;
				}

				return Reject(std::format(
				    "Cooked shader package '{}' is missing the required {}/{} binary for shader stage '{}'",
				    m_definition.PackageId,
				    CookedShaderBinaryFormatToString(m_requiredBinaryFormat),
				    m_requiredCodegenTarget,
				    GetShaderStagePrefix(stage)));
			}
			return true;
		}

		const LoadedShaderPackage& m_package;
		const ShaderPackageDefinition& m_definition;
		const PassParameterLayout& m_expectedBindingLayout;
		const std::vector<PassParameterDesc>& m_expectedParameters;
		CookedShaderBinaryFormat m_requiredBinaryFormat;
		std::string_view m_requiredCodegenTarget;
		std::uint64_t m_expectedBindingLayoutHash;
		std::string& m_errorMessage;
	};
}

bool CookedShaderPackageCache::ValidatePackage(
    const LoadedShaderPackage& package,
    const ShaderPackageDefinition& definition,
    const PassParameterLayout& expectedBindingLayout,
    CookedShaderBinaryFormat requiredBinaryFormat,
    std::string& outErrorMessage)
{
	return CookedShaderPackageValidator(
	           package, definition, expectedBindingLayout, requiredBinaryFormat, outErrorMessage)
	    .Validate();
}
