#pragma once

#include "../../../RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "../../../RHI/Public/Resources/RhiResourceDesc.h"
#include "../../../RHI/Public/Samplers/RhiSamplerDesc.h"
#include "../../../RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "../FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "../FrameGraph/FrameGraphAttachment.h"
#include "../FrameGraph/FrameGraphBufferHandle.h"
#include "../FrameGraph/FrameGraphTextureHandle.h"
#include "../RendererAPI.h"

#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

enum class PassParameterValueKind : std::uint8_t
{
	None,
	Texture,
	Buffer,
	DescriptorTable,
	AccelerationStructure,
	UniformData,
	Sampler,
};

struct PassParameterTextureBindingData
{
	std::vector<FrameGraphTextureHandle> Handles;
	FrameGraphAttachmentBinding Attachment = {};

	bool IsAttachment() const noexcept { return Attachment.Handle.IsValid(); }
	bool IsBound() const noexcept { return IsAttachment() || !Handles.empty(); }
};

struct PassParameterBufferBindingData
{
	std::vector<FrameGraphBufferHandle> Handles;

	bool IsBound() const noexcept { return !Handles.empty(); }
};

struct PassParameterDescriptorTableBindingData
{
	RhiDescriptorTableBinding Table = {};
	RhiGpuDescriptorHandle GpuHandle = {};

	bool IsBound() const noexcept { return static_cast<bool>(Table) || static_cast<bool>(GpuHandle); }
};

struct PassParameterUniformBindingData
{
	const void* Data = nullptr;
	std::uint32_t SizeInBytes = 0;

	bool IsBound() const noexcept { return Data != nullptr && SizeInBytes > 0; }
};

struct PassParameterSamplerBindingData
{
	RhiSamplerDesc Desc = {};

	bool IsBound() const noexcept { return true; }
};

using PassParameterBindingValue = std::variant<
    std::monostate,
    PassParameterTextureBindingData,
    PassParameterBufferBindingData,
    PassParameterDescriptorTableBindingData,
    FrameGraphAccelerationStructureHandle,
    PassParameterUniformBindingData,
    PassParameterSamplerBindingData>;

struct SPARKLE_RENDERER_API PassParameterBinding
{
	PassParameterValueKind GetKind() const noexcept;
	bool IsBound() const noexcept;

	const PassParameterTextureBindingData* AsTextureData() const noexcept { return std::get_if<PassParameterTextureBindingData>(&m_value); }

	const PassParameterBufferBindingData* AsBufferData() const noexcept { return std::get_if<PassParameterBufferBindingData>(&m_value); }

	const PassParameterDescriptorTableBindingData* AsDescriptorTableData() const noexcept
	{
		return std::get_if<PassParameterDescriptorTableBindingData>(&m_value);
	}

	const FrameGraphAccelerationStructureHandle* AsAccelerationStructureHandle() const noexcept
	{
		return std::get_if<FrameGraphAccelerationStructureHandle>(&m_value);
	}

	const PassParameterUniformBindingData* AsUniformData() const noexcept { return std::get_if<PassParameterUniformBindingData>(&m_value); }

	const PassParameterSamplerBindingData* AsSamplerData() const noexcept { return std::get_if<PassParameterSamplerBindingData>(&m_value); }

private:
	friend class PassParameterSet;

	void Reset() noexcept;
	void SetValue(PassParameterBindingValue value);

	PassParameterBindingValue m_value;
};

class SPARKLE_RENDERER_API PassParameterSet final
{
public:
	PassParameterSet(const PassParameterLayout& layout, std::vector<bool> graphResourceParameters);

	void ClearBindings() noexcept;

	const PassParameterLayout* GetLayout() const noexcept { return m_layout; }
	bool HasLayout() const noexcept { return m_layout != nullptr; }
	std::size_t GetBindingCount() const noexcept { return m_bindings.size(); }

	const PassParameterBinding* FindBinding(const char* name) const noexcept;
	const PassParameterBinding* GetBinding(std::uint32_t index) const noexcept;
	bool IsBound(const char* name) const noexcept;

	bool SetTexture(const char* name, FrameGraphTextureHandle handle);
	bool SetTextureArray(const char* name, const std::vector<FrameGraphTextureHandle>& handles);
	bool SetAttachment(const char* name, FrameGraphAttachmentBinding binding);
	bool SetBuffer(const char* name, FrameGraphBufferHandle handle);
	bool SetBufferArray(const char* name, const std::vector<FrameGraphBufferHandle>& handles);
	bool SetShaderResourceView(const char* name, RhiDescriptorTableBinding descriptorTable);
	bool SetShaderResourceView(const char* name, RhiGpuDescriptorHandle descriptorTable);
	bool UsesGraphResource(std::uint32_t index) const noexcept;
	bool SetUnorderedAccessView(const char* name, RhiDescriptorTableBinding descriptorTable);
	bool SetUnorderedAccessView(const char* name, RhiGpuDescriptorHandle descriptorTable);
	bool SetAccelerationStructure(const char* name, FrameGraphAccelerationStructureHandle handle);

	template <typename T> bool SetUniformDataReference(const char* name, const T& value)
	{
		static_assert(std::is_trivially_copyable_v<T>, "Uniform data must be trivially copyable.");
		static_assert(std::is_standard_layout_v<T>, "Uniform data must be standard layout.");

		return SetUniformDataBytes(name, &value, static_cast<std::uint32_t>(sizeof(T)));
	}

	bool SetUniformDataBytes(const char* name, const void* data, std::uint32_t sizeInBytes);
	bool SetSampler(const char* name, RhiSamplerDesc sampler);
	bool HasAllRequiredBindings() const noexcept;
	std::vector<std::string> GetMissingBindings() const;

private:
	const PassParameterDesc* FindParameter(const char* name, std::uint32_t& outIndex) const noexcept;
	const PassParameterBinding* FindBinding(const char* name, std::uint32_t& outIndex) const noexcept;
	bool SetDescriptorTable(
	    const char* name,
	    PassParameterDescriptorTableBindingData binding,
	    ShaderParameterSemanticKind textureKind,
	    ShaderParameterSemanticKind bufferKind);
	static bool ValidateArrayCount(const PassParameterDesc& parameter, std::size_t actualCount) noexcept;
	static bool ValidateTextureBinding(const std::vector<FrameGraphTextureHandle>& handles, const PassParameterDesc& parameter) noexcept;
	static bool ValidateBufferBinding(const std::vector<FrameGraphBufferHandle>& handles, const PassParameterDesc& parameter) noexcept;

	const PassParameterLayout* m_layout = nullptr;
	std::vector<PassParameterBinding> m_bindings;
	std::vector<bool> m_graphResourceParameters;
};
