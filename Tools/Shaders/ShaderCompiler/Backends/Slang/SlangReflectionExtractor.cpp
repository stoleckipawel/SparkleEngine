#include "PCH.h"

#include "Slang/SlangReflectionExtractor.h"

#include <algorithm>

ShaderReflection SlangReflectionExtractor::Extract(
    slang::ProgramLayout& programLayout,
    ShaderStage stage)
{
	ShaderReflection outReflection;

	if (slang::VariableLayoutReflection* globals = programLayout.getGlobalParamsVarLayout())
	{
		VisitScope(globals, stage, outReflection);
	}

	if (programLayout.getEntryPointCount() > 0)
	{
		slang::EntryPointReflection* entryPoint = programLayout.getEntryPointByIndex(0);
		if (entryPoint != nullptr)
		{
			if (stage == ShaderStage::Compute)
			{
				SlangUInt sizes[3] = {0, 0, 0};
				entryPoint->getComputeThreadGroupSize(3, sizes);
				outReflection.ThreadGroupSize = {
				    static_cast<std::uint32_t>(sizes[0]),
				    static_cast<std::uint32_t>(sizes[1]),
				    static_cast<std::uint32_t>(sizes[2])};
			}

			VisitScope(entryPoint->getVarLayout(), stage, outReflection);
		}
	}

	return outReflection;
}

void SlangReflectionExtractor::VisitScope(
    slang::VariableLayoutReflection* scopeLayout,
    ShaderStage stage,
    ShaderReflection& outReflection)
{
	if (scopeLayout == nullptr)
		return;

	VisitVariable(scopeLayout, stage, outReflection);
}

void SlangReflectionExtractor::VisitVariable(
    slang::VariableLayoutReflection* variableLayout,
    ShaderStage stage,
    ShaderReflection& outReflection)
{
	if (variableLayout == nullptr || variableLayout->getTypeLayout() == nullptr)
		return;

	for (unsigned categoryIndex = 0; categoryIndex < variableLayout->getCategoryCount(); ++categoryIndex)
	{
		const slang::ParameterCategory category = variableLayout->getCategoryByIndex(categoryIndex);
		switch (category)
		{
			case slang::ParameterCategory::ConstantBuffer:
			case slang::ParameterCategory::ShaderResource:
			case slang::ParameterCategory::UnorderedAccess:
			case slang::ParameterCategory::SamplerState:
			case slang::ParameterCategory::DescriptorTableSlot:
				AddResourceBinding(*variableLayout, category, outReflection);
				break;
			case slang::ParameterCategory::PushConstantBuffer:
				AddPushConstantBlock(*variableLayout, stage, outReflection);
				break;
			case slang::ParameterCategory::VaryingInput:
				if (stage == ShaderStage::Vertex)
				{
					AddVaryingInput(*variableLayout, outReflection);
				}
				break;
			default:
				break;
		}
	}

	VisitTypeFields(variableLayout->getTypeLayout(), stage, outReflection);
}

void SlangReflectionExtractor::VisitTypeFields(
    slang::TypeLayoutReflection* typeLayout,
    ShaderStage stage,
    ShaderReflection& outReflection)
{
	if (typeLayout == nullptr)
		return;

	slang::TypeLayoutReflection* unwrapped = UnwrapSingleElementContainer(typeLayout);
	if (unwrapped != nullptr && unwrapped != typeLayout)
	{
		VisitTypeFields(unwrapped, stage, outReflection);
		return;
	}

	if (typeLayout->getKind() != slang::TypeReflection::Kind::Struct)
		return;

	const unsigned fieldCount = typeLayout->getFieldCount();
	for (unsigned fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex)
	{
		VisitVariable(typeLayout->getFieldByIndex(fieldIndex), stage, outReflection);
	}
}

void SlangReflectionExtractor::AddResourceBinding(
    slang::VariableLayoutReflection& variableLayout,
    slang::ParameterCategory category,
    ShaderReflection& outReflection)
{
	ShaderReflectionResourceBinding binding;
	const char* bindingName = variableLayout.getName();
	if (category == slang::ParameterCategory::ConstantBuffer)
	{
		slang::TypeLayoutReflection* elementLayout = UnwrapSingleElementContainer(variableLayout.getTypeLayout());
		if (elementLayout == nullptr)
		{
			elementLayout = variableLayout.getTypeLayout();
		}

		if (elementLayout != nullptr)
		{
			const char* typeName = elementLayout->getName();
			if (typeName != nullptr && typeName[0] != '\0')
			{
				bindingName = typeName;
			}
		}
	}
	binding.Name = bindingName ? bindingName : "";
	binding.Kind = MapResourceKind(variableLayout.getTypeLayout(), category);
	binding.Dimension = MapResourceDimension(variableLayout.getTypeLayout());
	binding.IsReadOnly = binding.Kind != CookedShaderResourceKind::RWTexture &&
	                    binding.Kind != CookedShaderResourceKind::RWStructuredBuffer &&
	                    binding.Kind != CookedShaderResourceKind::RWByteAddressBuffer &&
	                    binding.Kind != CookedShaderResourceKind::RWTypedBuffer;
	binding.Set = static_cast<std::uint32_t>(variableLayout.getBindingSpace(category));
	binding.Slot = static_cast<std::uint32_t>(variableLayout.getOffset(category));
	binding.ArrayCount = NormalizeArrayCount(variableLayout.getTypeLayout()->getElementCount());

	if (binding.Kind == CookedShaderResourceKind::ConstantBuffer)
	{
		AddConstantBuffer(variableLayout, binding, outReflection);
	}

	const bool duplicate = std::ranges::any_of(
	    outReflection.Bindings,
	    [&binding](const ShaderReflectionResourceBinding& existing)
	    {
		    return existing.Name == binding.Name && existing.Kind == binding.Kind && existing.Set == binding.Set &&
		        existing.Slot == binding.Slot;
	    });
	if (!duplicate)
	{
		outReflection.Bindings.push_back(std::move(binding));
	}
}

void SlangReflectionExtractor::AddConstantBuffer(
    slang::VariableLayoutReflection& variableLayout,
    ShaderReflectionResourceBinding& binding,
    ShaderReflection& outReflection)
{
	ShaderReflectionConstantBuffer cb;
	cb.Name = binding.Name;
	slang::TypeLayoutReflection* elementLayout = UnwrapSingleElementContainer(variableLayout.getTypeLayout());
	if (elementLayout == nullptr)
	{
		elementLayout = variableLayout.getTypeLayout();
	}

	cb.SizeInBytes = static_cast<std::uint32_t>(elementLayout->getSize(slang::ParameterCategory::Uniform));
	FlattenMembers(elementLayout, 0u, cb.Members);
	binding.ConstantBufferIndex = static_cast<std::uint32_t>(outReflection.ConstantBuffers.size());
	binding.SizeInBytes = cb.SizeInBytes;
	outReflection.ConstantBuffers.push_back(std::move(cb));
}

void SlangReflectionExtractor::AddPushConstantBlock(
    slang::VariableLayoutReflection& variableLayout,
    ShaderStage stage,
    ShaderReflection& outReflection)
{
	ShaderReflectionPushConstantRange range;
	range.OffsetInBytes = static_cast<std::uint32_t>(variableLayout.getOffset(slang::ParameterCategory::PushConstantBuffer));
	range.SizeInBytes = static_cast<std::uint32_t>(variableLayout.getTypeLayout()->getSize(slang::ParameterCategory::Uniform));
	range.VisibilityMask = ToShaderStageMask(stage);
	outReflection.PushConstants.push_back(range);
}

void SlangReflectionExtractor::AddVaryingInput(
    slang::VariableLayoutReflection& variableLayout,
    ShaderReflection& outReflection)
{
	ShaderReflectionInputElement element;
	element.Semantic = variableLayout.getSemanticName() ? variableLayout.getSemanticName() : variableLayout.getName();
	element.SemanticIndex = static_cast<std::uint32_t>(variableLayout.getSemanticIndex());
	element.Location = static_cast<std::uint32_t>(variableLayout.getOffset(slang::ParameterCategory::VaryingInput));
	element.ScalarType = MapScalarType(variableLayout.getTypeLayout()->getScalarType());
	element.ComponentCount = static_cast<std::uint8_t>(std::max(1u, variableLayout.getTypeLayout()->getColumnCount()));
	outReflection.InputElements.push_back(std::move(element));
}

void SlangReflectionExtractor::FlattenMembers(
    slang::TypeLayoutReflection* typeLayout,
    std::uint32_t parentOffset,
    std::vector<ShaderReflectionConstantBufferMember>& outMembers)
{
	if (typeLayout == nullptr || typeLayout->getKind() != slang::TypeReflection::Kind::Struct)
		return;

	for (unsigned fieldIndex = 0; fieldIndex < typeLayout->getFieldCount(); ++fieldIndex)
	{
		slang::VariableLayoutReflection* field = typeLayout->getFieldByIndex(fieldIndex);
		if (field == nullptr || field->getTypeLayout() == nullptr)
			continue;

		ShaderReflectionConstantBufferMember member;
		member.Name = field->getName() ? field->getName() : "";
		member.OffsetInBytes = parentOffset + static_cast<std::uint32_t>(field->getOffset(slang::ParameterCategory::Uniform));
		member.SizeInBytes = static_cast<std::uint32_t>(field->getTypeLayout()->getSize(slang::ParameterCategory::Uniform));
		member.ArrayCount = NormalizeArrayCount(field->getTypeLayout()->getElementCount());
		member.ArrayStrideInBytes = static_cast<std::uint32_t>(
		    field->getTypeLayout()->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM));
		member.ScalarType = MapScalarType(field->getTypeLayout()->getScalarType());
		member.RowCount = static_cast<std::uint8_t>(field->getTypeLayout()->getRowCount());
		member.ColumnCount = static_cast<std::uint8_t>(field->getTypeLayout()->getColumnCount());
		outMembers.push_back(std::move(member));

		FlattenMembers(field->getTypeLayout(), member.OffsetInBytes, outMembers);
	}
}

CookedShaderResourceKind SlangReflectionExtractor::MapResourceKind(
    slang::TypeLayoutReflection* typeLayout,
    slang::ParameterCategory category)
{
	if (category == slang::ParameterCategory::ConstantBuffer)
		return CookedShaderResourceKind::ConstantBuffer;
	if (category == slang::ParameterCategory::SamplerState)
		return CookedShaderResourceKind::Sampler;

	const SlangResourceShape shape = typeLayout != nullptr ? typeLayout->getResourceShape() : SLANG_RESOURCE_NONE;
	const SlangResourceShape baseShape = static_cast<SlangResourceShape>(shape & SLANG_RESOURCE_BASE_SHAPE_MASK);
	const SlangResourceAccess access = typeLayout != nullptr ? typeLayout->getResourceAccess() : SLANG_RESOURCE_ACCESS_NONE;
	const bool writable = category == slang::ParameterCategory::UnorderedAccess || access != SLANG_RESOURCE_ACCESS_READ;

	if (baseShape == SLANG_STRUCTURED_BUFFER)
		return writable ? CookedShaderResourceKind::RWStructuredBuffer : CookedShaderResourceKind::StructuredBuffer;
	if (baseShape == SLANG_BYTE_ADDRESS_BUFFER)
		return writable ? CookedShaderResourceKind::RWByteAddressBuffer : CookedShaderResourceKind::ByteAddressBuffer;
	if (baseShape == SLANG_TEXTURE_BUFFER)
		return writable ? CookedShaderResourceKind::RWTypedBuffer : CookedShaderResourceKind::TypedBuffer;
	if (baseShape == SLANG_ACCELERATION_STRUCTURE)
		return CookedShaderResourceKind::AccelerationStructure;

	return writable ? CookedShaderResourceKind::RWTexture : CookedShaderResourceKind::Texture;
}

CookedShaderResourceDimension SlangReflectionExtractor::MapResourceDimension(slang::TypeLayoutReflection* typeLayout)
{
	const SlangResourceShape shape = typeLayout != nullptr ? typeLayout->getResourceShape() : SLANG_RESOURCE_NONE;
	const SlangResourceShape baseShape = static_cast<SlangResourceShape>(shape & SLANG_RESOURCE_BASE_SHAPE_MASK);
	const bool arrayed = (shape & SLANG_TEXTURE_ARRAY_FLAG) != 0;
	const bool multisampled = (shape & SLANG_TEXTURE_MULTISAMPLE_FLAG) != 0;

	switch (baseShape)
	{
		case SLANG_TEXTURE_1D:
			return arrayed ? CookedShaderResourceDimension::Texture1DArray : CookedShaderResourceDimension::Texture1D;
		case SLANG_TEXTURE_2D:
			if (multisampled)
				return arrayed ? CookedShaderResourceDimension::Texture2DMSArray : CookedShaderResourceDimension::Texture2DMS;
			return arrayed ? CookedShaderResourceDimension::Texture2DArray : CookedShaderResourceDimension::Texture2D;
		case SLANG_TEXTURE_3D:
			return CookedShaderResourceDimension::Texture3D;
		case SLANG_TEXTURE_CUBE:
			return arrayed ? CookedShaderResourceDimension::TextureCubeArray : CookedShaderResourceDimension::TextureCube;
		case SLANG_TEXTURE_BUFFER:
		case SLANG_STRUCTURED_BUFFER:
		case SLANG_BYTE_ADDRESS_BUFFER:
			return CookedShaderResourceDimension::Buffer;
		default:
			return CookedShaderResourceDimension::Unknown;
	}
}

CookedShaderScalarType SlangReflectionExtractor::MapScalarType(slang::TypeReflection::ScalarType type)
{
	switch (type)
	{
		case slang::TypeReflection::Bool:
			return CookedShaderScalarType::Bool;
		case slang::TypeReflection::Int16:
			return CookedShaderScalarType::Int16;
		case slang::TypeReflection::UInt16:
			return CookedShaderScalarType::UInt16;
		case slang::TypeReflection::Int32:
			return CookedShaderScalarType::Int32;
		case slang::TypeReflection::UInt32:
			return CookedShaderScalarType::UInt32;
		case slang::TypeReflection::Int64:
			return CookedShaderScalarType::Int64;
		case slang::TypeReflection::UInt64:
			return CookedShaderScalarType::UInt64;
		case slang::TypeReflection::Float16:
			return CookedShaderScalarType::Float16;
		case slang::TypeReflection::Float32:
			return CookedShaderScalarType::Float32;
		case slang::TypeReflection::Float64:
			return CookedShaderScalarType::Float64;
		default:
			return CookedShaderScalarType::Unknown;
	}
}

slang::TypeLayoutReflection* SlangReflectionExtractor::UnwrapSingleElementContainer(
    slang::TypeLayoutReflection* typeLayout)
{
	if (typeLayout == nullptr)
		return nullptr;

	switch (typeLayout->getKind())
	{
		case slang::TypeReflection::Kind::ConstantBuffer:
		case slang::TypeReflection::Kind::ParameterBlock:
		case slang::TypeReflection::Kind::TextureBuffer:
		case slang::TypeReflection::Kind::ShaderStorageBuffer:
			if (slang::VariableLayoutReflection* element = typeLayout->getElementVarLayout())
			{
				return element->getTypeLayout();
			}
			return typeLayout->getElementTypeLayout();
		default:
			return typeLayout;
	}
}

std::uint32_t SlangReflectionExtractor::NormalizeArrayCount(std::size_t elementCount)
{
	if (elementCount == 0 || elementCount == SLANG_UNKNOWN_SIZE)
		return 1;
	if (elementCount == SLANG_UNBOUNDED_SIZE)
		return 0;
	return static_cast<std::uint32_t>(elementCount);
}
