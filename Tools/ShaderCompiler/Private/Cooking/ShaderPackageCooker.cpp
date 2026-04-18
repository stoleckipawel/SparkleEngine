#include "PCH.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Cooking/ShaderCookManifest.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Compiler/DxcShaderCompiler.h"
#include "RHI/Public/Config/RenderConfig.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"
#include "RHI/Public/Shaders/ShaderCompileOptions.h"
#include "RHI/Public/Shaders/ShaderCompileResult.h"
#include "RHI/Public/Shaders/ShaderPackageLayoutCatalog.h"

#include <array>
#include <fstream>
#include <format>
#include <span>
#include <string_view>
#include <system_error>

namespace Engine::AssetAuthoring::Details
{
	struct CookedStageBuild final
	{
		ShaderStage stage = ShaderStage::Count;
		std::string sourcePath;
		std::string entryPoint;
		std::string debugArtifact;
		std::vector<std::uint8_t> bytecode;
		std::uint64_t bytecodeHash = 0;
	};

	struct StringTableBuilder final
	{
		CookedShaderStringRef Add(std::string_view value)
		{
			if (value.empty())
			{
				return {};
			}

			const std::uint32_t offset = static_cast<std::uint32_t>(bytes.size());
			bytes.insert(bytes.end(), value.begin(), value.end());
			return CookedShaderStringRef{offset, static_cast<std::uint32_t>(value.size())};
		}

		std::vector<std::uint8_t> bytes;
	};

	bool OpenBinaryOutput(const std::filesystem::path& path, std::ofstream& output, std::string& outErrorMessage)
	{
		std::error_code errorCode;
		std::filesystem::create_directories(path.parent_path(), errorCode);
		if (errorCode)
		{
			outErrorMessage = "Failed to create output directory '" + path.parent_path().string() + "'";
			return false;
		}

		output.open(path, std::ios::binary | std::ios::trunc);
		if (!output.is_open())
		{
			outErrorMessage = "Failed to open cooked shader package output '" + path.string() + "'";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	template <typename T>
	bool WriteValue(std::ofstream& output, const T& value, std::string& outErrorMessage)
	{
		output.write(reinterpret_cast<const char*>(&value), sizeof(T));
		if (output.good())
		{
			return true;
		}

		outErrorMessage = "Failed to write cooked shader payload";
		return false;
	}

	template <typename T>
	bool WriteArray(std::ofstream& output, const std::vector<T>& values, std::string& outErrorMessage)
	{
		if (values.empty())
		{
			return true;
		}

		output.write(reinterpret_cast<const char*>(values.data()), static_cast<std::streamsize>(sizeof(T) * values.size()));
		if (output.good())
		{
			return true;
		}

		outErrorMessage = "Failed to write cooked shader payload array";
		return false;
	}

	bool IsUnderRoot(const std::filesystem::path& path, const std::filesystem::path& root)
	{
		if (path.empty() || root.empty())
		{
			return false;
		}

		std::error_code errorCode;
		const std::filesystem::path relativePath = std::filesystem::relative(path, root, errorCode);
		if (errorCode)
		{
			return false;
		}

		const std::string relativePathString = relativePath.generic_string();
		return !relativePathString.empty() && !relativePathString.starts_with("..");
	}

	std::string SerializePathForArtifact(const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return {};
		}

		const std::filesystem::path normalizedPath = Engine::Paths::Normalize(path);
		const std::filesystem::path& projectRoot = Filesystem::GetProjectPath();
		if (IsUnderRoot(normalizedPath, projectRoot))
		{
			std::error_code errorCode;
			const std::filesystem::path relativePath = std::filesystem::relative(normalizedPath, projectRoot, errorCode);
			if (!errorCode)
			{
				return relativePath.generic_string();
			}
		}

		return normalizedPath.generic_string();
	}

	ShaderStageMask ToPackageStageMask(ShaderStageVisibility visibility) noexcept
	{
		switch (visibility)
		{
			case ShaderStageVisibility::Vertex:
				return ShaderStageMask::Vertex;
			case ShaderStageVisibility::Pixel:
				return ShaderStageMask::Pixel;
			case ShaderStageVisibility::Compute:
				return ShaderStageMask::Compute;
			case ShaderStageVisibility::AllGraphics:
				return ShaderStageMask::Vertex | ShaderStageMask::Pixel;
			case ShaderStageVisibility::All:
				return ShaderStageMask::Vertex | ShaderStageMask::Pixel | ShaderStageMask::Compute;
			case ShaderStageVisibility::None:
			default:
				return ShaderStageMask::None;
		}
	}

	std::string FormatStageMask(ShaderStageMask stageMask)
	{
		std::string value;
		const std::array<std::pair<ShaderStageMask, std::string_view>, 6> stageNames = {
		    std::pair{ShaderStageMask::Vertex, std::string_view{"Vertex"}},
		    std::pair{ShaderStageMask::Pixel, std::string_view{"Pixel"}},
		    std::pair{ShaderStageMask::Geometry, std::string_view{"Geometry"}},
		    std::pair{ShaderStageMask::Hull, std::string_view{"Hull"}},
		    std::pair{ShaderStageMask::Domain, std::string_view{"Domain"}},
		    std::pair{ShaderStageMask::Compute, std::string_view{"Compute"}}};

		for (const auto& [flag, name] : stageNames)
		{
			if (!HasAnyShaderStageMask(stageMask, flag))
			{
				continue;
			}

			if (!value.empty())
			{
				value += '|';
			}

			value += name;
		}

		return value.empty() ? std::string{"None"} : value;
	}

	std::uint64_t BuildSourceIdentityHash(
	    const ShaderCookPackageDesc& package,
	    std::span<const CookedStageBuild> compiledStages)
	{
		std::string canonical;
		canonical.reserve(256);
		canonical += package.packageId;
		canonical += '|';
		canonical += package.variantId;
		canonical += '|';
		canonical += package.bindingLayoutId;
		canonical += '|';
		canonical += std::to_string(RenderConfig::ShaderModelMajor);
		canonical += '.';
		canonical += std::to_string(RenderConfig::ShaderModelMinor);

		for (const CookedStageBuild& compiledStage : compiledStages)
		{
			canonical += ';';
			canonical += GetShaderStagePrefix(compiledStage.stage);
			canonical += '|';
			canonical += compiledStage.sourcePath;
			canonical += '|';
			canonical += compiledStage.entryPoint;
			canonical += '|';
			canonical += std::format("{:016X}", compiledStage.bytecodeHash);
		}

		const std::uint64_t hash = Hash::Fnv1a64(canonical);
		return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
	}

	ShaderCompileOptions BuildCompileOptions(const ShaderCookStageDesc& stage)
	{
		ShaderCompileOptions options{};
		options.SourcePath = Filesystem::ResolveAssetPathValidated(stage.sourcePath, AssetType::Shader);
		options.EntryPoint = stage.entryPoint;
		options.Stage = stage.stage;

		const std::filesystem::path& projectShaderRoot = Filesystem::GetShaderPath(PathRoot::Project);
		const std::filesystem::path& engineShaderRoot = Filesystem::GetShaderPath(PathRoot::Engine);

		if (!projectShaderRoot.empty())
		{
			options.IncludeDir = projectShaderRoot;
			if (!engineShaderRoot.empty() && Engine::Paths::MakePathKey(engineShaderRoot) != Engine::Paths::MakePathKey(projectShaderRoot))
			{
				options.AdditionalIncludeDirs.push_back(engineShaderRoot);
			}
		}
		else
		{
			options.IncludeDir = engineShaderRoot;
		}

	#if defined(ENGINE_SHADERS_DEBUG)
		options.EnableDebugInfo = true;
		options.StripDebugInfo = false;
	#endif

	#if defined(ENGINE_SHADERS_OPTIMIZED)
		options.EnableOptimizations = true;
	#else
		options.EnableOptimizations = true;
	#endif

		return options;
	}

	bool CompileStage(const ShaderCookStageDesc& stage, CookedStageBuild& outCompiledStage, std::string& outErrorMessage)
	{
		const ShaderCompileOptions options = BuildCompileOptions(stage);
		const ShaderCompileResult compileResult = DxcShaderCompiler::Compile(options);
		if (!compileResult.IsSuccess())
		{
			outErrorMessage = compileResult.GetErrorMessage();
			return false;
		}

		const ShaderBytecode bytecode = compileResult.GetBytecode();
		if (!bytecode.IsValid())
		{
			outErrorMessage = "DXC returned empty bytecode for shader source '" + stage.sourcePath.generic_string() + "'";
			return false;
		}

		const auto* bytecodeBegin = static_cast<const std::uint8_t*>(bytecode.Data);
		outCompiledStage.stage = stage.stage;
		outCompiledStage.sourcePath = stage.sourcePath.generic_string();
		outCompiledStage.entryPoint = stage.entryPoint;
		outCompiledStage.debugArtifact = SerializePathForArtifact(compileResult.GetDebugArtifactPath());
		outCompiledStage.bytecode.assign(bytecodeBegin, bytecodeBegin + bytecode.Size);
		outCompiledStage.bytecodeHash = Hash::Fnv1a64(outCompiledStage.bytecode.data(), outCompiledStage.bytecode.size());
		outErrorMessage.clear();
		return true;
	}

	bool BuildBindingRecords(
	    const PassParameterLayout& layout,
	    StringTableBuilder& stringTable,
	    std::vector<CookedShaderBindingRecord>& outBindingRecords)
	{
		const std::vector<PassParameterDesc>& parameters = layout.GetParameters();
		outBindingRecords.clear();
		outBindingRecords.reserve(parameters.size());

		for (std::size_t parameterIndex = 0; parameterIndex < parameters.size(); ++parameterIndex)
		{
			const PassParameterDesc& parameter = parameters[parameterIndex];
			outBindingRecords.push_back(
			    CookedShaderBindingRecord{
			        .Name = stringTable.Add(parameter.Name),
			        .SemanticKind = parameter.Kind,
			        .ResourceDomain = parameter.ResourceDomain,
			        .Access = parameter.Access,
			        .VisibilityMask = ToPackageStageMask(parameter.Visibility),
			        .LogicalBindingIndex = static_cast<std::uint32_t>(parameterIndex),
			        .ArrayCount = parameter.ArrayCount,
			        .ValueSizeInBytes = parameter.ValueSizeInBytes});
		}

		return true;
	}

	bool WritePackage(
	    const ShaderCookPackageDesc& package,
	    const PassParameterLayout& bindingLayout,
	    std::span<const CookedStageBuild> compiledStages,
	    CookedShaderPackageOutput& outPackageOutput,
	    std::string& outErrorMessage)
	{
		StringTableBuilder stringTable;
		std::vector<CookedShaderBinaryRecord> binaryRecords;
		std::vector<CookedShaderBindingRecord> bindingRecords;
		std::vector<CookedShaderSpecializationInputRecord> specializationInputs;
		std::vector<std::uint8_t> binaryBlob;

		if (!BuildBindingRecords(bindingLayout, stringTable, bindingRecords))
		{
			outErrorMessage = "Failed to build cooked shader binding records for package '" + package.packageId + "'";
			return false;
		}

		binaryRecords.reserve(compiledStages.size());
		binaryBlob.reserve(4096);
		ShaderStageMask declaredStages = ShaderStageMask::None;

		for (const CookedStageBuild& compiledStage : compiledStages)
		{
			const std::uint32_t blobOffset = static_cast<std::uint32_t>(binaryBlob.size());
			binaryBlob.insert(binaryBlob.end(), compiledStage.bytecode.begin(), compiledStage.bytecode.end());

			binaryRecords.push_back(
			    CookedShaderBinaryRecord{
			        .EntryPoint = stringTable.Add(compiledStage.entryPoint),
			        .DebugArtifact = stringTable.Add(compiledStage.debugArtifact),
			        .Bytecode = CookedShaderBlobRef{blobOffset, static_cast<std::uint32_t>(compiledStage.bytecode.size())},
			        .Stage = compiledStage.stage,
			        .Format = CookedShaderBinaryFormat::Dxil,
			        .BytecodeHash = compiledStage.bytecodeHash});

			declaredStages |= ToShaderStageMask(compiledStage.stage);
		}

		CookedShaderPackageHeader header{};
		header.DeclaredStages = declaredStages;
		header.ShaderModelMajor = static_cast<std::uint16_t>(RenderConfig::ShaderModelMajor);
		header.ShaderModelMinor = static_cast<std::uint16_t>(RenderConfig::ShaderModelMinor);
		header.BinaryRecordCount = static_cast<std::uint32_t>(binaryRecords.size());
		header.BindingRecordCount = static_cast<std::uint32_t>(bindingRecords.size());
		header.SpecializationInputCount = static_cast<std::uint32_t>(specializationInputs.size());
		header.StringTableSizeInBytes = static_cast<std::uint32_t>(stringTable.bytes.size());
		header.BinaryBlobSizeInBytes = static_cast<std::uint32_t>(binaryBlob.size());
		header.ShaderPackageKey = ShaderCookManifest::BuildShaderPackageKey(package.packageId, package.variantId);
		header.SourceIdentityHash = BuildSourceIdentityHash(package, compiledStages);
		header.BindingLayoutHash = BuildPassParameterLayoutHash(bindingLayout);
		header.VariantHash = BuildShaderVariantHash(package.variantId);

		const std::filesystem::path packagePath = ShaderCookManifest::BuildCookedShaderPackagePath(header.ShaderPackageKey);
		std::ofstream output;
		if (!OpenBinaryOutput(packagePath, output, outErrorMessage))
		{
			return false;
		}

		if (!WriteValue(output, header, outErrorMessage) || !WriteArray(output, binaryRecords, outErrorMessage) ||
		    !WriteArray(output, bindingRecords, outErrorMessage) || !WriteArray(output, specializationInputs, outErrorMessage) ||
		    !WriteArray(output, stringTable.bytes, outErrorMessage) || !WriteArray(output, binaryBlob, outErrorMessage))
		{
			return false;
		}

		outPackageOutput.packageId = package.packageId;
		outPackageOutput.variantId = package.variantId;
		outPackageOutput.bindingLayoutId = package.bindingLayoutId;
		outPackageOutput.outputPath = packagePath;
		outPackageOutput.packageKey = header.ShaderPackageKey;
		outPackageOutput.sourceIdentityHash = header.SourceIdentityHash;
		outPackageOutput.bindingLayoutHash = header.BindingLayoutHash;
		outPackageOutput.variantHash = header.VariantHash;
		outPackageOutput.declaredStages = header.DeclaredStages;
		outErrorMessage.clear();
		return true;
	}

	bool WriteRegistry(
	    std::span<const CookedShaderPackageOutput> packages,
	    std::filesystem::path& outRegistryPath,
	    std::string& outErrorMessage)
	{
		outRegistryPath = ShaderCookManifest::GetCookedShaderRegistryPath();
		std::error_code errorCode;
		std::filesystem::create_directories(outRegistryPath.parent_path(), errorCode);
		if (errorCode)
		{
			outErrorMessage = "Failed to create shader registry directory '" + outRegistryPath.parent_path().string() + "'";
			return false;
		}

		std::ofstream output(outRegistryPath, std::ios::trunc);
		if (!output.is_open())
		{
			outErrorMessage = "Failed to open shader registry output '" + outRegistryPath.string() + "'";
			return false;
		}

		output << "[ShaderPackageRegistry]\n";
		output << "Version = 1\n";
		output << "PackageCount = " << packages.size() << "\n\n";

		for (const CookedShaderPackageOutput& package : packages)
		{
			output << "[Package " << package.packageId << "]\n";
			output << "Variant = " << package.variantId << "\n";
			output << "BindingLayout = " << package.bindingLayoutId << "\n";
			output << "PackageKey = " << std::format("{:016X}", package.packageKey) << "\n";
			output << "SourceIdentityHash = " << std::format("{:016X}", package.sourceIdentityHash) << "\n";
			output << "BindingLayoutHash = " << std::format("{:016X}", package.bindingLayoutHash) << "\n";
			output << "VariantHash = " << std::format("{:016X}", package.variantHash) << "\n";
			output << "DeclaredStages = " << FormatStageMask(package.declaredStages) << "\n";
			output << "Output = " << SerializePathForArtifact(package.outputPath) << "\n\n";
		}

		if (!output.good())
		{
			outErrorMessage = "Failed to write shader registry output '" + outRegistryPath.string() + "'";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}
}

namespace Engine::AssetAuthoring
{
	ShaderPackageCookResult ShaderPackageCooker::CookAll() const
	{
		ShaderPackageCookResult result;
		ShaderCookManifest manifest;
		if (!manifest.LoadMerged(result.errorMessage))
		{
			return result;
		}

		result.packages.reserve(manifest.GetPackages().size());
		for (const ShaderCookPackageDesc& package : manifest.GetPackages())
		{
			PassParameterLayout bindingLayout;
			if (!ShaderPackageLayouts::TryBuild(package.bindingLayoutId, bindingLayout, result.errorMessage))
			{
				result.errorMessage = "Failed to build binding layout for shader package '" + package.packageId + "' - " + result.errorMessage;
				result.packages.clear();
				return result;
			}

			std::vector<Details::CookedStageBuild> compiledStages;
			compiledStages.reserve(package.stages.size());
			for (const ShaderCookStageDesc& stage : package.stages)
			{
				Details::CookedStageBuild compiledStage;
				if (!Details::CompileStage(stage, compiledStage, result.errorMessage))
				{
					result.errorMessage = std::format(
					    "Failed to compile shader package '{}' variant '{}' stage '{}' - {}",
					    package.packageId,
					    package.variantId,
					    GetShaderStagePrefix(stage.stage),
					    result.errorMessage);
					result.packages.clear();
					return result;
				}

				compiledStages.push_back(std::move(compiledStage));
			}

			CookedShaderPackageOutput packageOutput;
			if (!Details::WritePackage(package, bindingLayout, compiledStages, packageOutput, result.errorMessage))
			{
				result.errorMessage = "Failed to emit cooked shader package '" + package.packageId + "' - " + result.errorMessage;
				result.packages.clear();
				return result;
			}

			result.packages.push_back(std::move(packageOutput));
		}

		if (!Details::WriteRegistry(result.packages, result.registryPath, result.errorMessage))
		{
			result.packages.clear();
			return result;
		}

		return result;
	}
}