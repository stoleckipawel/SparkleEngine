#include "PCH.h"

#include "DxilReflectionExtractor.h"

#include <cstdint>
#include <wrl/client.h>

CookedShaderResourceKind DxilReflectionExtractor::MapResourceKind(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dim)
{
	switch (type)
	{
		case D3D_SIT_CBUFFER:
			return CookedShaderResourceKind::ConstantBuffer;
		case D3D_SIT_TBUFFER:
			return CookedShaderResourceKind::TypedBuffer;
		case D3D_SIT_TEXTURE:
			return CookedShaderResourceKind::Texture;
		case D3D_SIT_SAMPLER:
			return CookedShaderResourceKind::Sampler;
		case D3D_SIT_UAV_RWTYPED:
			return (dim == D3D_SRV_DIMENSION_BUFFER) ? CookedShaderResourceKind::RWTypedBuffer
			                                         : CookedShaderResourceKind::RWTexture;
		case D3D_SIT_STRUCTURED:
			return CookedShaderResourceKind::StructuredBuffer;
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			return CookedShaderResourceKind::RWStructuredBuffer;
		case D3D_SIT_BYTEADDRESS:
			return CookedShaderResourceKind::ByteAddressBuffer;
		case D3D_SIT_UAV_RWBYTEADDRESS:
			return CookedShaderResourceKind::RWByteAddressBuffer;
		case D3D_SIT_RTACCELERATIONSTRUCTURE:
			return CookedShaderResourceKind::AccelerationStructure;
		default:
			return CookedShaderResourceKind::Unknown;
	}
}

CookedShaderResourceDimension DxilReflectionExtractor::MapDimension(D3D_SRV_DIMENSION dim)
{
	switch (dim)
	{
		case D3D_SRV_DIMENSION_BUFFER:
			return CookedShaderResourceDimension::Buffer;
		case D3D_SRV_DIMENSION_TEXTURE1D:
			return CookedShaderResourceDimension::Texture1D;
		case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
			return CookedShaderResourceDimension::Texture1DArray;
		case D3D_SRV_DIMENSION_TEXTURE2D:
			return CookedShaderResourceDimension::Texture2D;
		case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
			return CookedShaderResourceDimension::Texture2DArray;
		case D3D_SRV_DIMENSION_TEXTURE2DMS:
			return CookedShaderResourceDimension::Texture2DMS;
		case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
			return CookedShaderResourceDimension::Texture2DMSArray;
		case D3D_SRV_DIMENSION_TEXTURE3D:
			return CookedShaderResourceDimension::Texture3D;
		case D3D_SRV_DIMENSION_TEXTURECUBE:
			return CookedShaderResourceDimension::TextureCube;
		case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
			return CookedShaderResourceDimension::TextureCubeArray;
		default:
			return CookedShaderResourceDimension::Unknown;
	}
}

bool DxilReflectionExtractor::IsReadOnlyKind(CookedShaderResourceKind kind)
{
	switch (kind)
	{
		case CookedShaderResourceKind::RWTexture:
		case CookedShaderResourceKind::RWStructuredBuffer:
		case CookedShaderResourceKind::RWByteAddressBuffer:
		case CookedShaderResourceKind::RWTypedBuffer:
			return false;
		default:
			return true;
	}
}

CookedShaderScalarType DxilReflectionExtractor::MapScalarType(D3D_SHADER_VARIABLE_TYPE type)
{
	switch (type)
	{
		case D3D_SVT_BOOL:
			return CookedShaderScalarType::Bool;
		case D3D_SVT_INT:
			return CookedShaderScalarType::Int32;
		case D3D_SVT_UINT:
			return CookedShaderScalarType::UInt32;
		case D3D_SVT_FLOAT:
			return CookedShaderScalarType::Float32;
		case D3D_SVT_INT16:
			return CookedShaderScalarType::Int16;
		case D3D_SVT_UINT16:
			return CookedShaderScalarType::UInt16;
		case D3D_SVT_FLOAT16:
			return CookedShaderScalarType::Float16;
		case D3D_SVT_DOUBLE:
			return CookedShaderScalarType::Float64;
		case D3D_SVT_INT64:
			return CookedShaderScalarType::Int64;
		case D3D_SVT_UINT64:
			return CookedShaderScalarType::UInt64;
		default:
			return CookedShaderScalarType::Unknown;
	}
}

CookedShaderScalarType DxilReflectionExtractor::MapInputComponent(D3D_REGISTER_COMPONENT_TYPE type)
{
	switch (type)
	{
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return CookedShaderScalarType::Float32;
		case D3D_REGISTER_COMPONENT_SINT32:
			return CookedShaderScalarType::Int32;
		case D3D_REGISTER_COMPONENT_UINT32:
			return CookedShaderScalarType::UInt32;
		default:
			return CookedShaderScalarType::Unknown;
	}
}

std::uint8_t DxilReflectionExtractor::PopMaskBits(std::uint8_t mask)
{
	std::uint8_t count = 0;
	while (mask)
	{
		count += static_cast<std::uint8_t>(mask & 1u);
		mask = static_cast<std::uint8_t>(mask >> 1u);
	}
	return count;
}

bool DxilReflectionExtractor::Extract(
    IDxcUtils& utils,
    IDxcResult* result,
    std::span<const std::uint8_t> bytecode,
    ShaderStage stage,
    ShaderReflection& outReflection,
    std::string& outError)
{
	outReflection = ShaderReflection{};
	outError.clear();
	(void)bytecode;

	if (result == nullptr)
	{
		outError = "DXIL reflection: null IDxcResult";
		return false;
	}

	// DXC exposes reflection as a separate output blob.
	// Query DXC_OUT_REFLECTION directly instead of walking the container.
	Microsoft::WRL::ComPtr<IDxcBlob> reflectionBlob;
	HRESULT hr = result->GetOutput(
	    DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionBlob.ReleaseAndGetAddressOf()), nullptr);
	if (FAILED(hr) || !reflectionBlob || reflectionBlob->GetBufferSize() == 0)
	{
		outError = "DXIL reflection: DXC produced no reflection part";
		return false;
	}

	DxcBuffer reflectionBuffer{};
	reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
	reflectionBuffer.Size = reflectionBlob->GetBufferSize();
	reflectionBuffer.Encoding = 0;

	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection;
	hr = utils.CreateReflection(&reflectionBuffer, IID_PPV_ARGS(shaderReflection.ReleaseAndGetAddressOf()));
	if (FAILED(hr) || !shaderReflection)
	{
		outError = "DXIL reflection: CreateReflection failed";
		return false;
	}

	D3D12_SHADER_DESC desc{};
	if (FAILED(shaderReflection->GetDesc(&desc)))
	{
		outError = "DXIL reflection: GetDesc failed";
		return false;
	}

	// Emit one record per reflected constant buffer.
	// Bindings resolve CB indices by name in the next loop.
	outReflection.ConstantBuffers.reserve(desc.ConstantBuffers);
	for (UINT i = 0; i < desc.ConstantBuffers; ++i)
	{
		ID3D12ShaderReflectionConstantBuffer* cb = shaderReflection->GetConstantBufferByIndex(i);
		if (cb == nullptr)
			continue;

		D3D12_SHADER_BUFFER_DESC cbDesc{};
		if (FAILED(cb->GetDesc(&cbDesc)))
			continue;

		ShaderReflectionConstantBuffer outCb;
		outCb.Name = cbDesc.Name ? cbDesc.Name : "";
		outCb.SizeInBytes = cbDesc.Size;
		outCb.Members.reserve(cbDesc.Variables);

		for (UINT v = 0; v < cbDesc.Variables; ++v)
		{
			ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(v);
			if (var == nullptr)
				continue;

			D3D12_SHADER_VARIABLE_DESC vDesc{};
			if (FAILED(var->GetDesc(&vDesc)))
				continue;

			ID3D12ShaderReflectionType* type = var->GetType();
			D3D12_SHADER_TYPE_DESC tDesc{};
			if (type != nullptr)
				type->GetDesc(&tDesc);

			ShaderReflectionConstantBufferMember member;
			member.Name = vDesc.Name ? vDesc.Name : "";
			member.OffsetInBytes = vDesc.StartOffset;
			member.SizeInBytes = vDesc.Size;
			member.ArrayCount = (tDesc.Elements > 0) ? tDesc.Elements : 1u;
			// D3D reflection does not expose array stride.
			// Approximate it as SizeInBytes / ArrayCount.
			member.ArrayStrideInBytes = (member.ArrayCount > 0) ? (member.SizeInBytes / member.ArrayCount) : member.SizeInBytes;
			member.ScalarType = MapScalarType(tDesc.Type);
			member.RowCount = static_cast<std::uint8_t>(tDesc.Rows);
			member.ColumnCount = static_cast<std::uint8_t>(tDesc.Columns);
			outCb.Members.push_back(std::move(member));
		}

		outReflection.ConstantBuffers.push_back(std::move(outCb));
	}

	// Resource bindings.
	outReflection.Bindings.reserve(desc.BoundResources);
	for (UINT i = 0; i < desc.BoundResources; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
		if (FAILED(shaderReflection->GetResourceBindingDesc(i, &bindDesc)))
			continue;

		ShaderReflectionResourceBinding binding;
		binding.Name = bindDesc.Name ? bindDesc.Name : "";
		binding.Kind = MapResourceKind(bindDesc.Type, bindDesc.Dimension);
		binding.Dimension = MapDimension(bindDesc.Dimension);
		binding.IsReadOnly = IsReadOnlyKind(binding.Kind);
		binding.Set = bindDesc.Space;
		binding.Slot = bindDesc.BindPoint;
		binding.ArrayCount = (bindDesc.BindCount == 0) ? 0u /* unbounded */ : bindDesc.BindCount;

		if (binding.Kind == CookedShaderResourceKind::ConstantBuffer)
		{
			binding.SizeInBytes = 0;
			for (std::size_t cbIndex = 0; cbIndex < outReflection.ConstantBuffers.size(); ++cbIndex)
			{
				if (outReflection.ConstantBuffers[cbIndex].Name == binding.Name)
				{
					binding.ConstantBufferIndex = static_cast<std::uint32_t>(cbIndex);
					binding.SizeInBytes = outReflection.ConstantBuffers[cbIndex].SizeInBytes;
					break;
				}
			}
		}
		else if (binding.Kind == CookedShaderResourceKind::StructuredBuffer ||
		         binding.Kind == CookedShaderResourceKind::RWStructuredBuffer)
		{
			binding.SizeInBytes = bindDesc.NumSamples; // element stride
		}

		outReflection.Bindings.push_back(std::move(binding));
	}

	// Vertex input signature: only meaningful for VS.
	if (stage == ShaderStage::Vertex)
	{
		outReflection.InputElements.reserve(desc.InputParameters);
		for (UINT i = 0; i < desc.InputParameters; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC paramDesc{};
			if (FAILED(shaderReflection->GetInputParameterDesc(i, &paramDesc)))
				continue;

			// Skip system-value semantics (SV_*) that are not consumed from
			// the input layout.
			if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED)
				continue;

			ShaderReflectionInputElement element;
			element.Semantic = paramDesc.SemanticName ? paramDesc.SemanticName : "";
			element.SemanticIndex = paramDesc.SemanticIndex;
			element.Location = paramDesc.Register;
			element.ScalarType = MapInputComponent(paramDesc.ComponentType);
			element.ComponentCount = PopMaskBits(paramDesc.Mask);
			outReflection.InputElements.push_back(std::move(element));
		}
	}

	// Compute thread-group size.
	if (stage == ShaderStage::Compute)
	{
		UINT x = 0, y = 0, z = 0;
		shaderReflection->GetThreadGroupSize(&x, &y, &z);
		outReflection.ThreadGroupSize = {x, y, z};
	}

	// DXIL does not expose push constants or specialization constants here.
	// Both arrays stay empty.

	return true;
}
