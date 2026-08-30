#include "PCH.h"

#include "SpirVReflectionExtractor.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cstdint>
#include <cstring>
#include <vector>

CookedShaderResourceKind SpirVReflectionExtractor::MapDescriptorType(
    SpvReflectDescriptorType type,
    SpvDim dim,
    SpvReflectDecorationFlags decorationFlags)
{
	switch (type)
	{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			return CookedShaderResourceKind::Sampler;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return (dim == SpvDimBuffer) ? CookedShaderResourceKind::TypedBuffer : CookedShaderResourceKind::Texture;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return (dim == SpvDimBuffer) ? CookedShaderResourceKind::RWTypedBuffer : CookedShaderResourceKind::RWTexture;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			return CookedShaderResourceKind::TypedBuffer;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			return CookedShaderResourceKind::RWTypedBuffer;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			return CookedShaderResourceKind::ConstantBuffer;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			return (decorationFlags & SPV_REFLECT_DECORATION_NON_WRITABLE) != 0u ? CookedShaderResourceKind::StructuredBuffer
			                                                                     : CookedShaderResourceKind::RWStructuredBuffer;
		case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			return CookedShaderResourceKind::AccelerationStructure;
		default:
			return CookedShaderResourceKind::Unknown;
	}
}

CookedShaderResourceDimension SpirVReflectionExtractor::MapImageDim(SpvDim dim, std::uint32_t arrayed, std::uint32_t ms)
{
	switch (dim)
	{
		case SpvDim1D:
			return arrayed ? CookedShaderResourceDimension::Texture1DArray : CookedShaderResourceDimension::Texture1D;
		case SpvDim2D:
			if (ms != 0u)
				return arrayed ? CookedShaderResourceDimension::Texture2DMSArray : CookedShaderResourceDimension::Texture2DMS;
			return arrayed ? CookedShaderResourceDimension::Texture2DArray : CookedShaderResourceDimension::Texture2D;
		case SpvDim3D:
			return CookedShaderResourceDimension::Texture3D;
		case SpvDimCube:
			return arrayed ? CookedShaderResourceDimension::TextureCubeArray : CookedShaderResourceDimension::TextureCube;
		case SpvDimBuffer:
			return CookedShaderResourceDimension::Buffer;
		default:
			return CookedShaderResourceDimension::Unknown;
	}
}

CookedShaderScalarType SpirVReflectionExtractor::MapInputFormat(SpvReflectFormat format, std::uint8_t& outComponentCount)
{
	switch (format)
	{
		case SPV_REFLECT_FORMAT_R32_UINT:
			outComponentCount = 1;
			return CookedShaderScalarType::UInt32;
		case SPV_REFLECT_FORMAT_R32_SINT:
			outComponentCount = 1;
			return CookedShaderScalarType::Int32;
		case SPV_REFLECT_FORMAT_R32_SFLOAT:
			outComponentCount = 1;
			return CookedShaderScalarType::Float32;
		case SPV_REFLECT_FORMAT_R32G32_UINT:
			outComponentCount = 2;
			return CookedShaderScalarType::UInt32;
		case SPV_REFLECT_FORMAT_R32G32_SINT:
			outComponentCount = 2;
			return CookedShaderScalarType::Int32;
		case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
			outComponentCount = 2;
			return CookedShaderScalarType::Float32;
		case SPV_REFLECT_FORMAT_R32G32B32_UINT:
			outComponentCount = 3;
			return CookedShaderScalarType::UInt32;
		case SPV_REFLECT_FORMAT_R32G32B32_SINT:
			outComponentCount = 3;
			return CookedShaderScalarType::Int32;
		case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
			outComponentCount = 3;
			return CookedShaderScalarType::Float32;
		case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
			outComponentCount = 4;
			return CookedShaderScalarType::UInt32;
		case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
			outComponentCount = 4;
			return CookedShaderScalarType::Int32;
		case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
			outComponentCount = 4;
			return CookedShaderScalarType::Float32;
		case SPV_REFLECT_FORMAT_R16_UINT:
			outComponentCount = 1;
			return CookedShaderScalarType::UInt16;
		case SPV_REFLECT_FORMAT_R16_SINT:
			outComponentCount = 1;
			return CookedShaderScalarType::Int16;
		case SPV_REFLECT_FORMAT_R16_SFLOAT:
			outComponentCount = 1;
			return CookedShaderScalarType::Float16;
		case SPV_REFLECT_FORMAT_R16G16_UINT:
			outComponentCount = 2;
			return CookedShaderScalarType::UInt16;
		case SPV_REFLECT_FORMAT_R16G16_SINT:
			outComponentCount = 2;
			return CookedShaderScalarType::Int16;
		case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
			outComponentCount = 2;
			return CookedShaderScalarType::Float16;
		case SPV_REFLECT_FORMAT_R16G16B16_UINT:
			outComponentCount = 3;
			return CookedShaderScalarType::UInt16;
		case SPV_REFLECT_FORMAT_R16G16B16_SINT:
			outComponentCount = 3;
			return CookedShaderScalarType::Int16;
		case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
			outComponentCount = 3;
			return CookedShaderScalarType::Float16;
		case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
			outComponentCount = 4;
			return CookedShaderScalarType::UInt16;
		case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
			outComponentCount = 4;
			return CookedShaderScalarType::Int16;
		case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
			outComponentCount = 4;
			return CookedShaderScalarType::Float16;
		case SPV_REFLECT_FORMAT_R64_UINT:
			outComponentCount = 1;
			return CookedShaderScalarType::UInt64;
		case SPV_REFLECT_FORMAT_R64_SINT:
			outComponentCount = 1;
			return CookedShaderScalarType::Int64;
		case SPV_REFLECT_FORMAT_R64_SFLOAT:
			outComponentCount = 1;
			return CookedShaderScalarType::Float64;
		default:
			outComponentCount = 0;
			return CookedShaderScalarType::Unknown;
	}
}

CookedShaderScalarType SpirVReflectionExtractor::MapNumericScalar(const SpvReflectNumericTraits& traits, bool isSigned)
{
	const std::uint32_t width = traits.scalar.width;
	if (width == 32u)
		return isSigned ? CookedShaderScalarType::Int32 : CookedShaderScalarType::UInt32;
	if (width == 16u)
		return isSigned ? CookedShaderScalarType::Int16 : CookedShaderScalarType::UInt16;
	if (width == 64u)
		return isSigned ? CookedShaderScalarType::Int64 : CookedShaderScalarType::UInt64;
	return CookedShaderScalarType::Unknown;
}

void SpirVReflectionExtractor::FlattenBlockMembers(
    const SpvReflectBlockVariable& block,
    std::uint32_t parentAbsoluteOffset,
    std::vector<ShaderReflectionConstantBufferMember>& outMembers)
{
	for (std::uint32_t i = 0; i < block.member_count; ++i)
	{
		const SpvReflectBlockVariable& m = block.members[i];

		ShaderReflectionConstantBufferMember member;
		member.Name = m.name ? m.name : "";
		member.OffsetInBytes = parentAbsoluteOffset + m.offset;
		member.SizeInBytes = m.size;
		member.ArrayCount = 1;
		for (std::uint32_t d = 0; d < m.array.dims_count; ++d)
		{
			member.ArrayCount *= (m.array.dims[d] == 0u ? 1u : m.array.dims[d]);
		}
		member.ArrayStrideInBytes = (m.array.stride != 0u)
		    ? m.array.stride
		    : ((member.ArrayCount > 0u) ? member.SizeInBytes / member.ArrayCount : member.SizeInBytes);

		// Map the type. Prefer numeric traits + type flags.
		if (m.type_description != nullptr)
		{
			const auto flags = m.type_description->type_flags;
			if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
			{
				const std::uint32_t width = m.numeric.scalar.width;
				member.ScalarType = (width == 16u) ? CookedShaderScalarType::Float16
				    : (width == 64u)               ? CookedShaderScalarType::Float64
				                                   : CookedShaderScalarType::Float32;
			}
			else if (flags & SPV_REFLECT_TYPE_FLAG_INT)
			{
				const bool isSigned = (m.numeric.scalar.signedness != 0u);
				member.ScalarType = MapNumericScalar(m.numeric, isSigned);
			}
			else if (flags & SPV_REFLECT_TYPE_FLAG_BOOL)
			{
				member.ScalarType = CookedShaderScalarType::Bool;
			}
			else
			{
				member.ScalarType = CookedShaderScalarType::Unknown;
			}

			if (flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
			{
				member.RowCount = static_cast<std::uint8_t>(m.numeric.matrix.row_count);
				member.ColumnCount = static_cast<std::uint8_t>(m.numeric.matrix.column_count);
			}
			else if (flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
			{
				member.RowCount = 1;
				member.ColumnCount = static_cast<std::uint8_t>(m.numeric.vector.component_count);
			}
			else
			{
				member.RowCount = 1;
				member.ColumnCount = 1;
			}
		}

		outMembers.push_back(std::move(member));

		// Recurse so the cooked record stores a fully flattened layout.
		// Offsets stay absolute so loaders do not need nested resolution.
		if (m.member_count > 0)
		{
			FlattenBlockMembers(m, parentAbsoluteOffset + m.offset, outMembers);
		}
	}
}

ShaderReflection SpirVReflectionExtractor::Extract(std::span<const std::uint8_t> bytecode, ShaderStage stage)
{
	ShaderReflection reflection;
	ShaderReflection& outReflection = reflection;

	if (bytecode.empty())
	{
		throw Diagnostics::Error("SPIR-V reflection: empty bytecode");
	}

	SpvReflectShaderModule module{};
	SpvReflectResult res = spvReflectCreateShaderModule(bytecode.size(), bytecode.data(), &module);
	if (res != SPV_REFLECT_RESULT_SUCCESS)
	{
		throw Diagnostics::Error("SPIR-V reflection: spvReflectCreateShaderModule failed");
	}

	// Emit CB records for uniform buffers plus resource bindings for all descriptors.
	// Binding entries point at the CB record when applicable.
	std::uint32_t bindingCount = 0;
	res = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
	std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount, nullptr);
	if (res == SPV_REFLECT_RESULT_SUCCESS && bindingCount > 0)
	{
		spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());
	}

	outReflection.Bindings.reserve(bindingCount);
	for (SpvReflectDescriptorBinding* b : bindings)
	{
		if (b == nullptr)
			continue;

		ShaderReflectionResourceBinding binding;
		binding.Name = (b->name && b->name[0] != '\0')
		    ? b->name
		    : (b->type_description && b->type_description->type_name ? b->type_description->type_name : "");
		binding.Kind = MapDescriptorType(b->descriptor_type, b->image.dim, b->decoration_flags);
		binding.Dimension = MapImageDim(b->image.dim, b->image.arrayed, b->image.ms);
		switch (binding.Kind)
		{
			case CookedShaderResourceKind::ConstantBuffer:
			case CookedShaderResourceKind::StructuredBuffer:
			case CookedShaderResourceKind::ByteAddressBuffer:
			case CookedShaderResourceKind::TypedBuffer:
			case CookedShaderResourceKind::RWStructuredBuffer:
			case CookedShaderResourceKind::RWByteAddressBuffer:
			case CookedShaderResourceKind::RWTypedBuffer:
				binding.Dimension = CookedShaderResourceDimension::Buffer;
				break;
			default:
				break;
		}
		binding.IsReadOnly =
		    (binding.Kind != CookedShaderResourceKind::RWTexture && binding.Kind != CookedShaderResourceKind::RWStructuredBuffer
		        && binding.Kind != CookedShaderResourceKind::RWByteAddressBuffer
		        && binding.Kind != CookedShaderResourceKind::RWTypedBuffer);
		binding.Set = b->set;
		binding.Slot = b->binding;
		binding.ArrayCount = (b->count == 0u) ? 1u : b->count;

		if (binding.Kind == CookedShaderResourceKind::ConstantBuffer)
		{
			ShaderReflectionConstantBuffer cb;
			cb.Name = binding.Name;
			cb.SizeInBytes = b->block.size;
			FlattenBlockMembers(b->block, 0u, cb.Members);
			binding.ConstantBufferIndex = static_cast<std::uint32_t>(outReflection.ConstantBuffers.size());
			binding.SizeInBytes = cb.SizeInBytes;
			outReflection.ConstantBuffers.push_back(std::move(cb));
		}

		outReflection.Bindings.push_back(std::move(binding));
	}

	// Vertex input variables.
	if (stage == ShaderStage::Vertex)
	{
		std::uint32_t inputCount = 0;
		spvReflectEnumerateInputVariables(&module, &inputCount, nullptr);
		std::vector<SpvReflectInterfaceVariable*> inputs(inputCount, nullptr);
		if (inputCount > 0)
			spvReflectEnumerateInputVariables(&module, &inputCount, inputs.data());

		outReflection.InputElements.reserve(inputCount);
		for (SpvReflectInterfaceVariable* v : inputs)
		{
			if (v == nullptr)
				continue;
			// Skip built-ins (SV_VertexID etc.); they're not in the input
			// layout the renderer needs to bind.
			if ((v->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) != 0u)
				continue;

			ShaderReflectionInputElement element;
			element.Semantic = (v->semantic && v->semantic[0] != '\0') ? v->semantic : (v->name ? v->name : "");
			element.SemanticIndex = 0;
			element.Location = v->location;
			std::uint8_t componentCount = 0;
			element.ScalarType = MapInputFormat(v->format, componentCount);
			element.ComponentCount = componentCount;
			outReflection.InputElements.push_back(std::move(element));
		}
	}

	// Push-constant blocks.
	{
		std::uint32_t pcCount = 0;
		spvReflectEnumeratePushConstantBlocks(&module, &pcCount, nullptr);
		std::vector<SpvReflectBlockVariable*> blocks(pcCount, nullptr);
		if (pcCount > 0)
			spvReflectEnumeratePushConstantBlocks(&module, &pcCount, blocks.data());

		const ShaderStageMask visibility = ToShaderStageMask(stage);
		for (SpvReflectBlockVariable* blk : blocks)
		{
			if (blk == nullptr)
				continue;

			ShaderReflectionPushConstantRange range;
			range.OffsetInBytes = blk->offset;
			range.SizeInBytes = blk->size;
			range.VisibilityMask = visibility;
			outReflection.PushConstants.push_back(range);

			// Also record the block layout in ConstantBuffers so renderers
			// can map members to root-constant slots.
			ShaderReflectionConstantBuffer cb;
			cb.Name = blk->name ? blk->name : "PushConstants";
			cb.SizeInBytes = blk->size;
			FlattenBlockMembers(*blk, 0u, cb.Members);

			ShaderReflectionResourceBinding binding;
			binding.Name = cb.Name;
			binding.Kind = CookedShaderResourceKind::PushConstantBlock;
			binding.Dimension = CookedShaderResourceDimension::Unknown;
			binding.IsReadOnly = true;
			binding.Set = 0;
			binding.Slot = 0;
			binding.ArrayCount = 1;
			binding.SizeInBytes = cb.SizeInBytes;
			binding.ConstantBufferIndex = static_cast<std::uint32_t>(outReflection.ConstantBuffers.size());
			outReflection.ConstantBuffers.push_back(std::move(cb));
			outReflection.Bindings.push_back(std::move(binding));
		}
	}

	// Specialization constants.
	{
		std::uint32_t specCount = 0;
		spvReflectEnumerateSpecializationConstants(&module, &specCount, nullptr);
		std::vector<SpvReflectSpecializationConstant*> specs(specCount, nullptr);
		if (specCount > 0)
			spvReflectEnumerateSpecializationConstants(&module, &specCount, specs.data());

		for (SpvReflectSpecializationConstant* s : specs)
		{
			if (s == nullptr)
				continue;

			ShaderReflectionSpecializationConstant sc;
			sc.Name = s->name ? s->name : "";
			sc.ConstantId = s->constant_id;
			// This SPIRV-Reflect release exposes only specialization constant id and name.
			// Type and default value are not surfaced here.
			sc.ScalarType = CookedShaderScalarType::Unknown;
			sc.DefaultValueBits = 0;
			outReflection.SpecializationConstants.push_back(std::move(sc));
		}
	}

	// Compute thread-group size (entry point 0; SPIR-V from DXC has one).
	if (stage == ShaderStage::Compute && module.entry_point_count > 0)
	{
		const auto& ep = module.entry_points[0];
		outReflection.ThreadGroupSize = {ep.local_size.x, ep.local_size.y, ep.local_size.z};
	}

	spvReflectDestroyShaderModule(&module);
	return reflection;
}
