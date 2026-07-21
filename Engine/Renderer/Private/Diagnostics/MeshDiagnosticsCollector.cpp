#include "PCH.h"

#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "Core/Public/Math/MathUtils.h"

#include "Meshes/GPUMeshCache.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshData.h"
#include "SceneData/Builders/MeshInstanceBatchBuilder.h"
#include "SceneData/RenderMeshClassificationConversion.h"
#include "SceneData/RenderWorld.h"

#include <algorithm>
#include <limits>
#include <unordered_map>

MeshDiagnosticsSnapshot MeshDiagnosticsCollector::Capture(const RenderWorld& world, const GPUMeshCache* gpuMeshCache)
{
	MeshDiagnosticsSnapshot snapshot;
	snapshot.Rows.reserve(world.GetProxies().size());

	std::unordered_map<const Mesh*, std::size_t> rowIndices;
	rowIndices.reserve(world.GetProxies().size());

	for (const auto& [object, proxy] : world.GetProxies())
	{
		const Mesh* mesh = proxy.Mesh.GetResource().get();
		if (mesh == nullptr)
		{
			continue;
		}

		MeshDiagnosticsRow* row = nullptr;
		if (auto it = rowIndices.find(mesh); it != rowIndices.end())
		{
			row = &snapshot.Rows[it->second];
		}
		else
		{
			MeshDiagnosticsRow newRow;
			PopulateMeshRow(newRow, *mesh, gpuMeshCache);
			rowIndices.emplace(mesh, snapshot.Rows.size());
			snapshot.Rows.push_back(newRow);
			row = &snapshot.Rows.back();
		}

		++row->InstanceCount;
		++row->VisibleInstanceCount;
		if (proxy.MeshKind == SceneMeshKind::Skeletal)
		{
			++row->SkinnedInstanceCount;
			row->HasSkeletonBinding = true;
			row->HasSkinInfluences = true;
		}

		const MaterialHandle materialHandle = proxy.Material;
		if (!row->HasMaterial && materialHandle.IsValid())
		{
			row->HasMaterial = true;
			row->FirstMaterialSlot = materialHandle.GetIndex();
		}
	}

	std::sort(
	    snapshot.Rows.begin(),
	    snapshot.Rows.end(),
	    [](const MeshDiagnosticsRow& lhs, const MeshDiagnosticsRow& rhs) noexcept
	    {
		    if (lhs.EstimatedGpuByteSize != rhs.EstimatedGpuByteSize)
		    {
			    return lhs.EstimatedGpuByteSize > rhs.EstimatedGpuByteSize;
		    }
		    if (lhs.EstimatedCpuByteSize != rhs.EstimatedCpuByteSize)
		    {
			    return lhs.EstimatedCpuByteSize > rhs.EstimatedCpuByteSize;
		    }
		    if (lhs.MeshAssetId != rhs.MeshAssetId)
		    {
			    return lhs.MeshAssetId < rhs.MeshAssetId;
		    }
		    return lhs.MeshRuntimeId < rhs.MeshRuntimeId;
	    });

	std::vector<MeshRenderItem> renderItems;
	renderItems.reserve(world.GetProxies().size());
	std::vector<RenderMeshInstanceGroup> renderInstanceGroups;
	for (const RenderMeshInstanceGroupData& group : world.GetInstanceGroups())
		renderInstanceGroups.push_back({RenderMeshClassificationConversion::ToRenderMeshInstanceGroupKind(group.Kind), group.InstanceCount});
	MeshGeometryInstancingDiagnostics& instancingDiagnostics = snapshot.GeometryInstancing;
	instancingDiagnostics.RuntimeInstanceGroupCount = static_cast<std::uint32_t>(renderInstanceGroups.size());
	for (const RenderMeshInstanceGroup& group : renderInstanceGroups)
	{
		if (group.groupKind == RenderMeshInstanceGroupKind::AuthoredInstanceGroup)
		{
			++instancingDiagnostics.RuntimeAuthoredGroupCount;
		}
		else if (group.groupKind == RenderMeshInstanceGroupKind::SharedMeshReference)
		{
			++instancingDiagnostics.RuntimeSharedMeshReferenceGroupCount;
		}
	}
	for (const auto& [object, proxy] : world.GetProxies())
	{
		const Mesh* mesh = proxy.Mesh.GetResource().get();
		if (mesh == nullptr)
		{
			continue;
		}

		const GPUMesh* gpuMesh = gpuMeshCache != nullptr ? gpuMeshCache->Find(*mesh) : nullptr;

		renderItems.push_back(
		    MeshRenderItem{
		        .draw = MeshDraw{
		            .Transform =
		                MeshDrawTransform{
		                    .WorldMatrix = MathUtils::IdentityFloat4x4(),
		                    .WorldInvTranspose = {}},
		            .Material =
		                MeshDrawMaterial{
		                    .Slot = proxy.Material.IsValid() ? proxy.Material.GetIndex() : 0u},
		            .Skinning =
		                MeshDrawSkinning{
		                    .SkeletonAssetId = proxy.SkeletonAssetId},
		            .Source =
		                MeshDrawSourceIdentity{
		                    .SourceInstanceIndex = static_cast<std::uint32_t>(renderItems.size())},
		                    .Geometry =
		                        MeshDrawGeometry{
		                            .MeshKind = RenderMeshClassificationConversion::ToRenderMeshKind(proxy.MeshKind),
		                            .GpuMesh = gpuMesh}},
		        .instanceGroupIndex = RenderMeshClassificationConversion::ToRenderMeshInstanceGroupIndex(proxy.InstanceGroupIndex)});
	}

	MeshInstanceBatchBuilder batchBuilder;
	MeshGeometryInstancingDiagnostics batchDiagnostics = batchBuilder.Build(
	    renderItems,
	    renderInstanceGroups,
	    MeshInstanceBatchBuildOptions{
	        .enableAutoBatching = CVarRendererMeshAutoBatching.Get(),
	        .requireMaterialBindingSet = false,
	        .collectDiagnostics = true})
	                                  .diagnostics;
	batchDiagnostics.RuntimeInstanceGroupCount = instancingDiagnostics.RuntimeInstanceGroupCount;
	batchDiagnostics.RuntimeAuthoredGroupCount = instancingDiagnostics.RuntimeAuthoredGroupCount;
	batchDiagnostics.RuntimeSharedMeshReferenceGroupCount = instancingDiagnostics.RuntimeSharedMeshReferenceGroupCount;
	snapshot.GeometryInstancing = batchDiagnostics;

	return snapshot;
}

MeshPreviewGeometry MeshDiagnosticsCollector::CapturePreview(const RenderWorld& world, std::uintptr_t meshRuntimeId)
{
	MeshPreviewGeometry geometry;
	if (meshRuntimeId == 0) return geometry;
	for (const auto& [object, proxy] : world.GetProxies())
	{
		const Mesh* mesh = proxy.Mesh.GetResource().get();
		if (mesh == nullptr || reinterpret_cast<std::uintptr_t>(mesh) != meshRuntimeId) continue;
		const MeshData& data = mesh->GetMeshData();
		geometry.Vertices.reserve(data.vertices.size());
		for (const VertexData& vertex : data.vertices)
			geometry.Vertices.push_back({vertex.position.x, vertex.position.y, vertex.position.z});
		geometry.Indices.assign(data.indices.begin(), data.indices.end());
		break;
	}
	return geometry;
}

void MeshDiagnosticsCollector::PopulateMeshRow(MeshDiagnosticsRow& row, const Mesh& mesh, const GPUMeshCache* gpuMeshCache)
{
	const MeshData& meshData = mesh.GetMeshData();
	if (const CookedMesh* cookedMesh = dynamic_cast<const CookedMesh*>(&mesh))
	{
		row.MeshAssetId = cookedMesh->GetAssetId();
	}

	row.MeshRuntimeId = reinterpret_cast<std::uintptr_t>(&mesh);
	row.CpuLoaded = meshData.IsValid();
	row.VertexCount = meshData.GetVertexCount();
	row.IndexCount = meshData.GetIndexCount();
	row.TriangleCount = row.IndexCount / 3u;
	row.VertexStrideBytes = static_cast<std::uint32_t>(sizeof(VertexData));
	row.IndexStrideBytes = static_cast<std::uint32_t>(sizeof(std::uint32_t));
	row.EstimatedCpuByteSize = static_cast<std::uint64_t>(meshData.GetVertexBufferSize() + meshData.GetIndexBufferSize());

	if (!meshData.vertices.empty())
	{
		float minX = (std::numeric_limits<float>::max)();
		float minY = (std::numeric_limits<float>::max)();
		float minZ = (std::numeric_limits<float>::max)();
		float maxX = (std::numeric_limits<float>::lowest)();
		float maxY = (std::numeric_limits<float>::lowest)();
		float maxZ = (std::numeric_limits<float>::lowest)();
		for (const VertexData& vertex : meshData.vertices)
		{
			minX = (std::min) (minX, vertex.position.x);
			minY = (std::min) (minY, vertex.position.y);
			minZ = (std::min) (minZ, vertex.position.z);
			maxX = (std::max) (maxX, vertex.position.x);
			maxY = (std::max) (maxY, vertex.position.y);
			maxZ = (std::max) (maxZ, vertex.position.z);
		}
		row.Bounds.Min = {minX, minY, minZ};
		row.Bounds.Max = {maxX, maxY, maxZ};
		row.Bounds.IsValid = true;
	}

	if (gpuMeshCache == nullptr)
	{
		return;
	}

	if (const GPUMesh* gpuMesh = gpuMeshCache->Find(mesh))
	{
		row.GpuMeshRuntimeId = reinterpret_cast<std::uintptr_t>(gpuMesh);
		row.GpuResident = gpuMesh->IsValid();
		row.ResidencyState = row.GpuResident ? MeshDiagnosticsResidencyState::Resident : MeshDiagnosticsResidencyState::Unloaded;
		row.EstimatedGpuByteSize = static_cast<std::uint64_t>(gpuMesh->GetVertexBufferView().SizeInBytes) +
		                          static_cast<std::uint64_t>(gpuMesh->GetIndexBufferView().SizeInBytes);
	}
}
