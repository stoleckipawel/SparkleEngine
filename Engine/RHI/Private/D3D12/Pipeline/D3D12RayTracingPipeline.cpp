#include "PCH.h"

#include "D3D12/Pipeline/D3D12RayTracingPipeline.h"

#include "Core/Public/Diagnostics/Error.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Validation/RhiContract.h"

#include <format>
#include <string>
#include <vector>

class D3D12RayTracingPipelineAssembly final
{
public:
	explicit D3D12RayTracingPipelineAssembly(const RayTracingPipelineDesc& desc)
	{
		RhiContract::ValidateRayTracingPipelineDesc(desc);
		m_exportNames.reserve(desc.ShaderExports.size());
		m_exportDescs.reserve(desc.ShaderExports.size());
		m_libraryDescs.reserve(desc.ShaderExports.size());
		m_hitGroupNames.reserve(desc.HitGroups.size());
		m_closestHitNames.reserve(desc.HitGroups.size());
		m_anyHitNames.reserve(desc.HitGroups.size());
		m_intersectionNames.reserve(desc.HitGroups.size());
		m_hitGroupDescs.reserve(desc.HitGroups.size());
		m_subobjects.reserve(desc.ShaderExports.size() + desc.HitGroups.size() + 3);

		for (const RhiRayTracingShaderExportDesc& shaderExport : desc.ShaderExports)
		{
			const ShaderBytecode bytecode = shaderExport.Shader->GetBytecode();
			if (shaderExport.Shader->Entry->BinaryFormat != ShaderBinaryFormat::Dxil || !bytecode.IsValid())
			{
				throw Diagnostics::Error("D3D12 ray-tracing pipeline requires valid DXIL library code.");
			}
			m_exportNames.push_back(Strings::ToWide(shaderExport.ExportName));
			m_exportDescs.push_back(
			    D3D12_EXPORT_DESC{.Name = m_exportNames.back().c_str(), .ExportToRename = nullptr, .Flags = D3D12_EXPORT_FLAG_NONE});
			m_libraryDescs.push_back(
			    D3D12_DXIL_LIBRARY_DESC{
			        .DXILLibrary = D3D12_SHADER_BYTECODE{.pShaderBytecode = bytecode.Data, .BytecodeLength = bytecode.Size},
			        .NumExports = 1,
			        .pExports = &m_exportDescs.back()});
			m_subobjects.push_back(D3D12_STATE_SUBOBJECT{.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &m_libraryDescs.back()});
		}

		for (const RhiRayTracingHitGroupDesc& hitGroup : desc.HitGroups)
		{
			m_hitGroupNames.push_back(Strings::ToWide(hitGroup.ExportName));
			m_closestHitNames.push_back(Strings::ToWide(hitGroup.ClosestHitExport));
			m_anyHitNames.push_back(Strings::ToWide(hitGroup.AnyHitExport));
			m_intersectionNames.push_back(Strings::ToWide(hitGroup.IntersectionExport));
			m_hitGroupDescs.push_back(
			    D3D12_HIT_GROUP_DESC{
			        .HitGroupExport = m_hitGroupNames.back().c_str(),
			        .Type = hitGroup.Kind == ERhiRayTracingHitGroupKind::Triangles ? D3D12_HIT_GROUP_TYPE_TRIANGLES
			                                                                       : D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
			        .AnyHitShaderImport = hitGroup.AnyHitExport.empty() ? nullptr : m_anyHitNames.back().c_str(),
			        .ClosestHitShaderImport = hitGroup.ClosestHitExport.empty() ? nullptr : m_closestHitNames.back().c_str(),
			        .IntersectionShaderImport = hitGroup.IntersectionExport.empty() ? nullptr : m_intersectionNames.back().c_str()});
			m_subobjects.push_back(D3D12_STATE_SUBOBJECT{.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, .pDesc = &m_hitGroupDescs.back()});
		}

		m_shaderConfig = D3D12_RAYTRACING_SHADER_CONFIG{
		    .MaxPayloadSizeInBytes = desc.MaxPayloadSizeInBytes,
		    .MaxAttributeSizeInBytes = desc.MaxAttributeSizeInBytes};
		m_subobjects.push_back(
		    D3D12_STATE_SUBOBJECT{.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, .pDesc = &m_shaderConfig});
		const auto* bindingLayout = dynamic_cast<const D3D12BindingLayout*>(desc.GlobalBindingLayout);
		if (bindingLayout == nullptr)
		{
			throw Diagnostics::Error("D3D12 ray-tracing pipeline received a foreign global binding layout.");
		}
		m_globalRootSignature = bindingLayout->GetRootSignature().GetRaw();
		m_subobjects.push_back(
		    D3D12_STATE_SUBOBJECT{.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &m_globalRootSignature});
		m_pipelineConfig = D3D12_RAYTRACING_PIPELINE_CONFIG{.MaxTraceRecursionDepth = desc.MaxRecursionDepth};
		m_subobjects.push_back(
		    D3D12_STATE_SUBOBJECT{.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, .pDesc = &m_pipelineConfig});
	}

	D3D12_STATE_OBJECT_DESC BuildDesc() const noexcept
	{
		return D3D12_STATE_OBJECT_DESC{
		    .Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		    .NumSubobjects = static_cast<UINT>(m_subobjects.size()),
		    .pSubobjects = m_subobjects.data()};
	}

private:
	std::vector<std::wstring> m_exportNames;
	std::vector<D3D12_EXPORT_DESC> m_exportDescs;
	std::vector<D3D12_DXIL_LIBRARY_DESC> m_libraryDescs;
	std::vector<std::wstring> m_hitGroupNames;
	std::vector<std::wstring> m_closestHitNames;
	std::vector<std::wstring> m_anyHitNames;
	std::vector<std::wstring> m_intersectionNames;
	std::vector<D3D12_HIT_GROUP_DESC> m_hitGroupDescs;
	D3D12_RAYTRACING_SHADER_CONFIG m_shaderConfig = {};
	ID3D12RootSignature* m_globalRootSignature = nullptr;
	D3D12_RAYTRACING_PIPELINE_CONFIG m_pipelineConfig = {};
	std::vector<D3D12_STATE_SUBOBJECT> m_subobjects;
};

D3D12RayTracingPipeline::D3D12RayTracingPipeline(D3D12Rhi& rhi, const RayTracingPipelineDesc& desc) :
    RayTracingPipeline(desc)
{
	const RhiRayTracingCapabilities capabilities = rhi.GetRayTracingCapabilities();
	if (!capabilities.SupportsRayTracingPipeline || desc.MaxPayloadSizeInBytes > capabilities.MaxRayPayloadSizeInBytes
	    || desc.MaxAttributeSizeInBytes > capabilities.MaxRayAttributeSizeInBytes
	    || desc.MaxRecursionDepth > capabilities.MaxTraceRecursionDepth)
	{
		throw Diagnostics::Error("D3D12 ray-tracing pipeline exceeds device readiness or limits.");
	}
	D3D12RayTracingPipelineAssembly assembly(desc);
	const D3D12_STATE_OBJECT_DESC stateObjectDesc = assembly.BuildDesc();
	const HRESULT result = rhi.GetDevice()->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(m_stateObject.ReleaseAndGetAddressOf()));
	if (FAILED(result) || !m_stateObject || FAILED(m_stateObject.As(&m_properties)) || !m_properties)
	{
		throw Diagnostics::Error(std::format("D3D12 ray-tracing state-object creation failed: 0x{:08X}.", static_cast<unsigned>(result)));
	}
	if (desc.DebugName != nullptr)
	{
		m_stateObject->SetName(desc.DebugName);
	}
}

const void* D3D12RayTracingPipeline::FindShaderIdentifier(std::string_view exportName) const noexcept
{
	const std::wstring wideName = Strings::ToWide(exportName);
	return m_properties != nullptr ? m_properties->GetShaderIdentifier(wideName.c_str()) : nullptr;
}
