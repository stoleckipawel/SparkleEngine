#include "PCH.h"

#include "Shaders/CookedShaderPackageCache.h"

#include "Core/Public/Diagnostics/Error.h"
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

class CookedShaderPackageValidationImplementation final
{
  public:
	static constexpr std::array<ShaderStage, 6> kKnownShaderStages =
	    {ShaderStage::Vertex, ShaderStage::Pixel, ShaderStage::Geometry, ShaderStage::Hull, ShaderStage::Domain, ShaderStage::Compute};

	class CookedShaderPackageValidator final
	{
	  public:
		CookedShaderPackageValidator(
		    const LoadedShaderPackage& package,
		    const ShaderPackageDefinition& definition,
		    const PassParameterLayout& expectedBindingLayout,
		    CookedShaderBinaryFormat runtimeBinaryFormat) noexcept
		    : m_package(package),
		      m_definition(definition),
		      m_expectedBindingLayout(expectedBindingLayout),
		      m_expectedParameters(expectedBindingLayout.GetParameters()),
		      m_runtimeBinaryFormat(runtimeBinaryFormat),
		      m_runtimeCodegenTarget(GetRuntimeShaderCodegenTarget(runtimeBinaryFormat)),
		      m_expectedBindingLayoutHash(BuildPassParameterLayoutHash(expectedBindingLayout))
		{
		}

		void Validate()
		{
			ValidateHeader();
			ValidateRayTracingFeatures();
			ValidatePipelineLayouts();
			ValidateDeclaredStages();
			ValidateLogicalBindingRecords();
			CookedShaderBindingValidation::Validate(
			    m_package, m_definition, m_expectedParameters, m_runtimeBinaryFormat);
			ValidateBinaries();
		}

	  private:
		[[noreturn]] static void Reject(std::string message)
		{
			throw Diagnostics::Error(std::move(message));
		}

		void ValidateHeader()
		{
			if (!m_package.IsValid())
			{
				Reject("Cooked shader package payload is invalid.");
			}

			const std::uint64_t expectedPackageKey = BuildShaderPackageKey(m_definition.PackageId);
			if (m_package.GetHeader().ShaderPackageKey != expectedPackageKey)
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: field=ShaderPackageKey expected={} actual={}",
				    m_definition.PackageId,
				    Formatting::FormatHexUInt64(expectedPackageKey),
				    Formatting::FormatHexUInt64(m_package.GetHeader().ShaderPackageKey)));
			}

			if (m_package.GetHeader().SourceIdentityHash == 0)
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: field=SourceIdentityHash actual=0",
				    m_definition.PackageId));
			}

			if (m_package.GetHeader().ShaderModelMajor != CookedShaderPackageContract::ShaderModelMajor ||
			    m_package.GetHeader().ShaderModelMinor != CookedShaderPackageContract::ShaderModelMinor)
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: field=ShaderModel expected={}.{} actual={}.{}",
				    m_definition.PackageId,
				    CookedShaderPackageContract::ShaderModelMajor,
				    CookedShaderPackageContract::ShaderModelMinor,
				    m_package.GetHeader().ShaderModelMajor,
				    m_package.GetHeader().ShaderModelMinor));
			}
		}

		void ValidateRayTracingFeatures()
		{
			std::string metadataError;
			if (m_package.GetHeader().PackageKind == CookedShaderPackageKind::RayTracingLibrary)
			{
				RhiRayTracingCapabilities metadataOnlyCapabilities{};
				metadataOnlyCapabilities.SupportsRayTracing = true;
				if (!m_package.ValidateRayTracingLibraryMetadata(metadataOnlyCapabilities, m_runtimeBinaryFormat, metadataError))
				{
					Reject(std::move(metadataError));
				}

				Reject(std::format(
				    "Cooked shader package '{}' is a ray tracing library package with valid metadata, but runtime RT state object execution is not enabled yet.",
				    m_definition.PackageId));
			}

			if (HasCookedShaderPackageFeature(
			        m_package.GetHeader().PackageFeatures, CookedShaderPackageFeatureFlags::UsesInlineRayQuery) &&
			    !ShaderRayTracingMetadataValidation::ValidateInlineRayQueryMetadata(
			        m_package, m_runtimeBinaryFormat, metadataError))
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed inline ray query metadata validation: {}",
				    m_definition.PackageId,
				    metadataError));
			}
		}

		void ValidatePipelineLayouts()
		{
			if (m_package.GetHeader().BindingLayoutHash != m_expectedBindingLayoutHash)
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: field=BindingLayoutHash bindingLayout='{}' expected={} actual={}",
				    m_definition.PackageId,
				    m_expectedBindingLayout.GetDebugName(),
				    Formatting::FormatHexUInt64(m_expectedBindingLayoutHash),
				    Formatting::FormatHexUInt64(m_package.GetHeader().BindingLayoutHash)));
			}

			if (m_package.GetPipelineLayoutRecords().empty())
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: no cooked pipeline layout artifact records were found. Recook shaders.",
				    m_definition.PackageId));
			}

			std::uint32_t runtimeLayoutRecordCount = 0;
			for (const CookedShaderPipelineLayoutRecord& layoutRecord : m_package.GetPipelineLayoutRecords())
			{
				ValidatePipelineLayoutRecord(layoutRecord, runtimeLayoutRecordCount);
			}

			if (runtimeLayoutRecordCount != 1)
			{
				Reject(std::format(
				    "Cooked shader package '{}' requires exactly one pipeline layout for runtime target '{}' but contains {}",
				    m_definition.PackageId,
				    m_runtimeCodegenTarget,
				    runtimeLayoutRecordCount));
			}
		}

		void ValidatePipelineLayoutRecord(const CookedShaderPipelineLayoutRecord& layoutRecord, std::uint32_t& runtimeRecordCount)
		{
			if (layoutRecord.BindingLayoutHash != m_expectedBindingLayoutHash)
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: pipeline layout BindingLayoutHash expected={} actual={}",
				    m_definition.PackageId,
				    Formatting::FormatHexUInt64(m_expectedBindingLayoutHash),
				    Formatting::FormatHexUInt64(layoutRecord.BindingLayoutHash)));
			}

			if (layoutRecord.BindingRecordOffset + layoutRecord.BindingRecordCount > m_package.GetBindingRecords().size())
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: pipeline layout binding range is out of bounds",
				    m_definition.PackageId));
			}

			const std::string_view codegenTarget = m_package.ResolveString(layoutRecord.CodegenTarget);
			if (codegenTarget.empty())
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed package contract check: pipeline layout CodegenTarget is empty",
				    m_definition.PackageId));
			}

			runtimeRecordCount += codegenTarget == m_runtimeCodegenTarget ? 1u : 0u;
		}

		void ValidateDeclaredStages()
		{
			if (CookedShaderBindingRules::HasAllStages(m_package.GetHeader().DeclaredStages, m_definition.ExpectedStages))
			{
				return;
			}

			Reject(std::format(
			    "Cooked shader package '{}' failed package contract check: field=DeclaredStages expected='{}' actual='{}'",
			    m_definition.PackageId,
			    FormatShaderStageMask(m_definition.ExpectedStages),
			    FormatShaderStageMask(m_package.GetHeader().DeclaredStages)));
		}

		void ValidateLogicalBindingRecords()
		{
			if (m_package.GetBindingRecords().size() != m_expectedParameters.size())
			{
				Reject(std::format(
				    "Cooked shader package '{}' declares {} binding records but runtime layout '{}' expects {}",
				    m_definition.PackageId,
				    m_package.GetBindingRecords().size(),
				    m_expectedBindingLayout.GetDebugName(),
				    m_expectedParameters.size()));
			}

			for (std::size_t parameterIndex = 0; parameterIndex < m_expectedParameters.size(); ++parameterIndex)
			{
				ValidateLogicalBindingRecord(parameterIndex);
			}
		}

		void ValidateLogicalBindingRecord(std::size_t parameterIndex)
		{
			const PassParameterDesc& expectedParameter = m_expectedParameters[parameterIndex];
			const CookedShaderBindingRecord& bindingRecord = m_package.GetBindingRecords()[parameterIndex];
			const std::string_view bindingName = m_package.ResolveString(bindingRecord.Name);
			if (bindingName.empty())
			{
				Reject(std::format(
				    "Cooked shader package '{}' has an invalid binding name string for binding index {}",
				    m_definition.PackageId,
				    parameterIndex));
			}

			const bool matches = bindingRecord.LogicalBindingIndex == parameterIndex && bindingName == expectedParameter.Name &&
			                     bindingRecord.SemanticKind == expectedParameter.Kind &&
			                     bindingRecord.ResourceDomain == expectedParameter.ResourceDomain &&
			                     bindingRecord.Access == expectedParameter.Access &&
			                     bindingRecord.VisibilityMask == CookedShaderBindingRules::ToPackageStageMask(expectedParameter.Visibility) &&
			                     bindingRecord.ArrayCount == expectedParameter.ArrayCount &&
			                     bindingRecord.ValueSizeInBytes == expectedParameter.ValueSizeInBytes;
			if (matches)
			{
				return;
			}

			Reject(std::format(
			    "Cooked shader package '{}' binding record {} does not match runtime layout parameter '{}'",
			    m_definition.PackageId,
			    parameterIndex,
			    expectedParameter.Name));
		}

		void ValidateBinaries()
		{
			std::array<bool, static_cast<std::size_t>(ShaderStage::Count)> hasRuntimeBinaryForStage = {};
			for (const CookedShaderBinaryRecord& binaryRecord : m_package.GetBinaryRecords())
			{
				ValidateBinaryRecord(binaryRecord, hasRuntimeBinaryForStage);
			}

			ValidateExpectedStages(hasRuntimeBinaryForStage);
		}

		void ValidateBinaryRecord(
		    const CookedShaderBinaryRecord& binaryRecord,
		    std::array<bool, static_cast<std::size_t>(ShaderStage::Count)>& hasRuntimeBinaryForStage)
		{
			if (binaryRecord.Stage == ShaderStage::Count)
			{
				Reject(std::format(
				    "Cooked shader package '{}' contains an invalid shader stage record", m_definition.PackageId));
			}
			if (m_package.ResolveString(binaryRecord.EntryPoint).empty())
			{
				Reject(std::format(
				    "Cooked shader package '{}' contains an invalid entry point string", m_definition.PackageId));
			}
			if (binaryRecord.DebugArtifact && m_package.ResolveString(binaryRecord.DebugArtifact).empty())
			{
				Reject(std::format(
				    "Cooked shader package '{}' contains an invalid debug artifact string", m_definition.PackageId));
			}
			if (m_package.ResolveString(binaryRecord.CodegenTarget).empty())
			{
				Reject(std::format(
				    "Cooked shader package '{}' contains a binary with an invalid CodegenTarget string", m_definition.PackageId));
			}

			const ShaderBytecode bytecode = m_package.GetBytecode(binaryRecord);
			if (!bytecode.IsValid())
			{
				Reject(std::format(
				    "Cooked shader package '{}' contains an invalid bytecode blob for stage {}",
				    m_definition.PackageId,
				    static_cast<std::uint32_t>(binaryRecord.Stage)));
			}
			if (Hash::Fnv1a64(bytecode.Data, bytecode.Size) != binaryRecord.BytecodeHash)
			{
				Reject(std::format(
				    "Cooked shader package '{}' failed bytecode hash validation for stage {}",
				    m_definition.PackageId,
				    static_cast<std::uint32_t>(binaryRecord.Stage)));
			}
			if (!CookedShaderBindingRules::HasAllStages(
			        m_package.GetHeader().DeclaredStages,
			        ToShaderStageMask(binaryRecord.Stage)))
			{
				Reject(std::format(
				    "Cooked shader package '{}' contains a stage record outside its declared stage mask", m_definition.PackageId));
			}

			if (!m_package.IsRuntimeBinary(binaryRecord, m_runtimeBinaryFormat))
			{
				return;
			}

			const std::size_t stageIndex = static_cast<std::size_t>(binaryRecord.Stage);
			if (hasRuntimeBinaryForStage[stageIndex])
			{
				Reject(std::format(
				    "Cooked shader package '{}' contains more than one {}/{} binary for stage {}",
				    m_definition.PackageId,
				    CookedShaderBinaryFormatToString(m_runtimeBinaryFormat),
				    m_runtimeCodegenTarget,
				    stageIndex));
			}

			hasRuntimeBinaryForStage[stageIndex] = true;
		}

		void ValidateExpectedStages(const std::array<bool, static_cast<std::size_t>(ShaderStage::Count)>& availableStages)
		{
			for (const ShaderStage stage : kKnownShaderStages)
			{
				if (!CookedShaderBindingRules::HasAllStages(m_definition.ExpectedStages, ToShaderStageMask(stage)) ||
				    availableStages[static_cast<std::size_t>(stage)])
				{
					continue;
				}

				Reject(std::format(
				    "Cooked shader package '{}' is missing the runtime {}/{} binary for shader stage '{}'",
				    m_definition.PackageId,
				    CookedShaderBinaryFormatToString(m_runtimeBinaryFormat),
				    m_runtimeCodegenTarget,
				    GetShaderStagePrefix(stage)));
			}
		}

		const LoadedShaderPackage& m_package;
		const ShaderPackageDefinition& m_definition;
		const PassParameterLayout& m_expectedBindingLayout;
		const std::vector<PassParameterDesc>& m_expectedParameters;
		CookedShaderBinaryFormat m_runtimeBinaryFormat;
		std::string_view m_runtimeCodegenTarget;
		std::uint64_t m_expectedBindingLayoutHash;
	};
};

void CookedShaderPackageCache::ValidatePackage(
    const LoadedShaderPackage& package,
    const ShaderPackageDefinition& definition,
    const PassParameterLayout& expectedBindingLayout,
    CookedShaderBinaryFormat runtimeBinaryFormat)
{
	CookedShaderPackageValidationImplementation::CookedShaderPackageValidator(
	    package, definition, expectedBindingLayout, runtimeBinaryFormat)
	    .Validate();
}
