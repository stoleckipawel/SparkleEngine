#include "PCH.h"

#include "DxilReflectionExtractor.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <wrl/client.h>

ShaderReflection DxilReflectionExtractor::ExtractLibrary(IDxcUtils& utils, IDxcResult* result, std::string_view entryPoint)
{
	if (result == nullptr || entryPoint.empty())
	{
		throw Diagnostics::Error("DXIL library reflection requires a compile result and export name.");
	}

	Microsoft::WRL::ComPtr<IDxcBlob> reflectionBlob;
	if (FAILED(result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionBlob.ReleaseAndGetAddressOf()), nullptr)) || !reflectionBlob
	    || reflectionBlob->GetBufferSize() == 0)
	{
		throw Diagnostics::Error("DXIL library reflection: DXC produced no reflection part.");
	}
	const DxcBuffer reflectionBuffer{.Ptr = reflectionBlob->GetBufferPointer(), .Size = reflectionBlob->GetBufferSize(), .Encoding = 0};
	Microsoft::WRL::ComPtr<ID3D12LibraryReflection> library;
	if (FAILED(utils.CreateReflection(&reflectionBuffer, IID_PPV_ARGS(library.ReleaseAndGetAddressOf()))) || !library)
	{
		throw Diagnostics::Error("DXIL library reflection: CreateReflection failed.");
	}
	D3D12_LIBRARY_DESC libraryDesc{};
	if (FAILED(library->GetDesc(&libraryDesc)))
	{
		throw Diagnostics::Error("DXIL library reflection: GetDesc failed.");
	}

	ID3D12FunctionReflection* function = nullptr;
	D3D12_FUNCTION_DESC functionDesc{};
	for (UINT functionIndex = 0; functionIndex < libraryDesc.FunctionCount; ++functionIndex)
	{
		ID3D12FunctionReflection* candidate = library->GetFunctionByIndex(functionIndex);
		D3D12_FUNCTION_DESC candidateDesc{};
		if (candidate != nullptr && SUCCEEDED(candidate->GetDesc(&candidateDesc)) && candidateDesc.Name != nullptr
		    && std::string_view(candidateDesc.Name) == entryPoint)
		{
			function = candidate;
			functionDesc = candidateDesc;
			break;
		}
	}
	if (function == nullptr)
	{
		throw Diagnostics::Error("DXIL library reflection did not contain the requested export.");
	}

	ShaderReflection reflection;
	reflection.ConstantBuffers.reserve(functionDesc.ConstantBuffers);
	for (UINT bufferIndex = 0; bufferIndex < functionDesc.ConstantBuffers; ++bufferIndex)
	{
		ID3D12ShaderReflectionConstantBuffer* buffer = function->GetConstantBufferByIndex(bufferIndex);
		D3D12_SHADER_BUFFER_DESC bufferDesc{};
		if (buffer == nullptr || FAILED(buffer->GetDesc(&bufferDesc)))
		{
			throw Diagnostics::Error("DXIL library reflection could not read a constant-buffer declaration.");
		}
		ShaderReflectionConstantBuffer reflectedBuffer;
		reflectedBuffer.Name = bufferDesc.Name != nullptr ? bufferDesc.Name : "";
		reflectedBuffer.SizeInBytes = bufferDesc.Size;
		reflectedBuffer.Members.reserve(bufferDesc.Variables);
		for (UINT variableIndex = 0; variableIndex < bufferDesc.Variables; ++variableIndex)
		{
			ID3D12ShaderReflectionVariable* variable = buffer->GetVariableByIndex(variableIndex);
			D3D12_SHADER_VARIABLE_DESC variableDesc{};
			if (variable == nullptr || FAILED(variable->GetDesc(&variableDesc)))
			{
				throw Diagnostics::Error("DXIL library reflection could not read a constant-buffer member.");
			}
			D3D12_SHADER_TYPE_DESC typeDesc{};
			ID3D12ShaderReflectionType* type = variable->GetType();
			if (type == nullptr || FAILED(type->GetDesc(&typeDesc)))
			{
				throw Diagnostics::Error("DXIL library reflection could not read a constant-buffer member type.");
			}
			const std::uint32_t arrayCount = typeDesc.Elements > 0 ? typeDesc.Elements : 1u;
			reflectedBuffer.Members.push_back(
			    ShaderReflectionConstantBufferMember{
			        .Name = variableDesc.Name != nullptr ? variableDesc.Name : "",
			        .OffsetInBytes = variableDesc.StartOffset,
			        .SizeInBytes = variableDesc.Size,
			        .ArrayCount = arrayCount,
			        .ArrayStrideInBytes = variableDesc.Size / arrayCount,
			        .ScalarType = MapScalarType(typeDesc.Type),
			        .RowCount = static_cast<std::uint8_t>(typeDesc.Rows),
			        .ColumnCount = static_cast<std::uint8_t>(typeDesc.Columns)});
		}
		reflection.ConstantBuffers.push_back(std::move(reflectedBuffer));
	}

	reflection.Bindings.reserve(functionDesc.BoundResources);
	for (UINT bindingIndex = 0; bindingIndex < functionDesc.BoundResources; ++bindingIndex)
	{
		D3D12_SHADER_INPUT_BIND_DESC bindingDesc{};
		if (FAILED(function->GetResourceBindingDesc(bindingIndex, &bindingDesc)))
		{
			throw Diagnostics::Error("DXIL library reflection could not read a resource binding.");
		}
		ShaderReflectionResourceBinding binding;
		binding.Name = bindingDesc.Name != nullptr ? bindingDesc.Name : "";
		binding.Kind = MapResourceKind(bindingDesc.Type, bindingDesc.Dimension);
		binding.Dimension = MapDimension(bindingDesc.Dimension);
		binding.IsReadOnly = IsReadOnlyKind(binding.Kind);
		binding.Set = bindingDesc.Space;
		binding.Slot = bindingDesc.BindPoint;
		binding.ArrayCount = bindingDesc.BindCount == 0 ? 0u : bindingDesc.BindCount;
		if (binding.Kind == CookedShaderResourceKind::ConstantBuffer)
		{
			for (std::size_t bufferIndex = 0; bufferIndex < reflection.ConstantBuffers.size(); ++bufferIndex)
			{
				if (reflection.ConstantBuffers[bufferIndex].Name == binding.Name)
				{
					binding.ConstantBufferIndex = static_cast<std::uint32_t>(bufferIndex);
					binding.SizeInBytes = reflection.ConstantBuffers[bufferIndex].SizeInBytes;
					break;
				}
			}
		}
		else if (binding.Kind == CookedShaderResourceKind::StructuredBuffer || binding.Kind == CookedShaderResourceKind::RWStructuredBuffer)
		{
			binding.SizeInBytes = bindingDesc.NumSamples;
		}
		reflection.Bindings.push_back(std::move(binding));
	}
	return reflection;
}
