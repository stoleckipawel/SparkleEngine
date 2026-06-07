#include "PCH.h"

#include "Diagnostics/MeshDiagnosticsCollector.h"

#include "Meshes/GPUMeshCache.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/MeshData.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "SceneData/Builders/MeshInstanceBatchBuilder.h"

#include <algorithm>
#include <limits>
#include <unordered_map>

MeshDiagnosticsSnapshot MeshDiagnosticsCollector::Capture(const SceneMeshes& sceneMeshes, const GPUMeshCache* gpuMeshCache)
{
	MeshDiagnosticsSnapshot snapshot;
	snapshot.Rows.reserve(sceneMeshes.GetMeshCount());

	std::unordered_map<const Mesh*, std::size_t> rowIndices;
	rowIndices.reserve(sceneMeshes.GetMeshCount());

	for (std::size_t meshIndex = 0; meshIndex < sceneMeshes.GetMeshCount(); ++meshIndex)
	{
		const MeshComponent* meshComponent = sceneMeshes.GetMeshComponent(meshIndex);
		if (meshComponent == nullptr)
		{
			continue;
		}

		const Mesh* mesh = meshComponent->GetMesh();
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
		if (meshComponent->IsVisible())
		{
			++row->VisibleInstanceCount;
		}
		if (meshComponent->IsSkeletalMeshComponent())
		{
			++row->SkinnedInstanceCount;
			row->HasSkeletonBinding = true;
			row->HasSkinInfluences = true;
		}

		const MaterialHandle materialHandle = meshComponent->GetMaterialHandle();
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

	const MeshSnapshot meshSnapshot = sceneMeshes.CaptureSnapshot();
	std::vector<MeshRenderItem> renderItems;
	renderItems.reserve(meshSnapshot.meshInstances.size());
	MeshGeometryInstancingDiagnostics& instancingDiagnostics = snapshot.GeometryInstancing;
	instancingDiagnostics.RuntimeInstanceGroupCount = static_cast<std::uint32_t>(meshSnapshot.meshInstanceGroups.size());
	for (const MeshInstanceGroupSnapshot& group : meshSnapshot.meshInstanceGroups)
	{
		if (group.groupKind == SceneMeshInstanceGroupKind::AuthoredInstanceGroup)
		{
			++instancingDiagnostics.RuntimeAuthoredGroupCount;
		}
		else if (group.groupKind == SceneMeshInstanceGroupKind::SharedMeshReference)
		{
			++instancingDiagnostics.RuntimeSharedMeshReferenceGroupCount;
		}
	}
	for (const MeshInstanceSnapshot& meshInstance : meshSnapshot.meshInstances)
	{
		if (meshInstance.mesh == nullptr)
		{
			continue;
		}

		const GPUMesh* gpuMesh = gpuMeshCache != nullptr ? gpuMeshCache->Find(*meshInstance.mesh) : nullptr;
		SceneMeshInstanceGroupKind instanceGroupKind = SceneMeshInstanceGroupKind::None;
		if (meshInstance.instanceGroupIndex < meshSnapshot.meshInstanceGroups.size())
		{
			instanceGroupKind = meshSnapshot.meshInstanceGroups[meshInstance.instanceGroupIndex].groupKind;
		}

		renderItems.push_back(
		    MeshRenderItem{
		        .draw = MeshDraw{
		            .worldMatrix = meshInstance.worldMatrix,
		            .worldInvTranspose = meshInstance.worldInvTranspose,
		            .materialSlot = meshInstance.materialHandle.IsValid() ? meshInstance.materialHandle.GetIndex() : 0u,
		            .skeletonAssetId = meshInstance.skeletonAssetId,
		            .meshKind = meshInstance.meshKind,
		            .gpuMesh = gpuMesh},
		        .instanceGroupIndex = meshInstance.instanceGroupIndex,
		        .instanceGroupKind = instanceGroupKind,
		        .sourceInstanceIndex = static_cast<std::uint32_t>(renderItems.size())});
	}

	MeshInstanceBatchBuilder batchBuilder;
	MeshGeometryInstancingDiagnostics batchDiagnostics = batchBuilder.Build(
	    renderItems,
	    meshSnapshot.meshInstanceGroups,
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
