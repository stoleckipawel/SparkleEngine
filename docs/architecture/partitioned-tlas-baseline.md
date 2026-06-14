# Partitioned TLAS Baseline Evidence

Status: Stage 1 evidence freeze. Runtime code must not change in this stage.

Related plan: [Partitioned TLAS Implementation Plan](../plans/partitioned-tlas-implementation-plan.md)

## Purpose

Partitioned TLAS support is a provider-selection architecture challenge. The renderer needs one conceptual feature: build and update the top-level ray tracing scene efficiently. Backends expose that feature through different provider surfaces:

- Classic TLAS baseline provider.
- Vulkan NVIDIA PTLAS provider.
- D3D12 NVIDIA NVAPI PTLAS provider.
- Future public D3D12 DXR PTLAS provider.

This document freezes the evidence used before runtime implementation starts. Later stages should update this file with implemented provider status, validation artifacts, and capture links.

## Reference Patterns

| Pattern | Reference | Baseline decision |
|---|---|---|
| Capability-first provider selection | Vulkan `VK_NV_partitioned_acceleration_structure`, NVAPI PTLAS caps, Microsoft DXR Part 2 PTLAS docs | Query support before allocating PTLAS resources. Report active provider and capability status reason. |
| Classic TLAS remains comparable | Current Sparkle classic TLAS path | Keep classic TLAS selectable on every backend as the correctness and performance baseline. |
| Visual explanation is part of the feature | NVIDIA `vk_partitioned_tlas` partition colors/update highlights, RTXMG-style profiler/debug UI | Require viewmodes, overlays, screenshots, and capture markers as acceptance artifacts. |
| Renderer owns scene policy | NVIDIA sample separates partition/update policy from native API submission | Renderer emits logical partition/update intent. RHI owns native Vulkan/NVAPI records. |
| GPU-driven updates are the target | NVIDIA sample writes PTLAS operation data from compute | CPU pack is a bring-up/reference path. GPU-written logical and native operations are the review-ready target. |

## API Availability Evidence

| Provider | Evidence | Go/no-go state |
|---|---|---|
| Classic TLAS | Existing Sparkle D3D12 and Vulkan classic TLAS paths | Go. This is the baseline provider. |
| Vulkan NVIDIA PTLAS | Local Vulkan SDK `C:/VulkanSDK/1.4.350.0/Include/vulkan/vulkan_core.h` exposes `VK_NV_partitioned_acceleration_structure`, `VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV`, `vkGetPartitionedAccelerationStructuresBuildSizesNV`, and `vkCmdBuildPartitionedAccelerationStructuresNV`. | Go when runtime device extension, feature query, function loading, descriptor path, and validation succeed. |
| D3D12 NVIDIA NVAPI PTLAS | Research copy `build/research/nvapi/nvapi.h` exposes `NvAPI_D3D12_GetRaytracingCaps`, `NVAPI_D3D12_RAYTRACING_CAPS_TYPE_PARTITIONED_TLAS`, `NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAP_STANDARD`, `NvAPI_D3D12_GetRaytracingPartitionedTlasIndirectPrebuildInfo`, `NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect`, and `NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP`. | Go when NVAPI headers are wired, NVAPI initializes, ray tracing caps report standard PTLAS support, and D3D12 device/command list interfaces match requirements. |
| Future public D3D12 DXR PTLAS | Microsoft DXR Part 2 documents partitioned TLAS through RTAS operations, but local Windows SDK `10.0.26100.0` `d3d12.h` does not expose `D3D12_RTAS_PARTITIONED_TLAS*`, `ExecuteIndirectRTASOperations`, or public PTLAS symbols. | No-go for this checkout. Keep a clean inactive-provider path and add later behind the same RHI contract. |

## Native API Names To Preserve

Vulkan provider:

- `VK_NV_partitioned_acceleration_structure`
- `VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV`
- `VkWriteDescriptorSetPartitionedAccelerationStructureNV`
- `VkPartitionedAccelerationStructureWriteInstanceDataNV`
- `VkBuildPartitionedAccelerationStructureIndirectCommandNV`
- `vkGetPartitionedAccelerationStructuresBuildSizesNV`
- `vkCmdBuildPartitionedAccelerationStructuresNV`

D3D12 NVAPI provider:

- `NvAPI_D3D12_GetRaytracingCaps`
- `NVAPI_D3D12_RAYTRACING_CAPS_TYPE_PARTITIONED_TLAS`
- `NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAP_STANDARD`
- `NvAPI_D3D12_GetRaytracingPartitionedTlasIndirectPrebuildInfo`
- `NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect`
- `NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_INPUTS`
- `NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_DESC`
- `NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP`
- `NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_WRITE_INSTANCE`
- `NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_UPDATE_INSTANCE`
- `NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_WRITE_PARTITION`

## Current Sparkle Classic TLAS Ownership

Renderer ownership:

- `Engine/Renderer/Private/RayTracing/RenderRayTracingScene.cpp`
- `Engine/Renderer/Private/RayTracing/RenderRayTracingScene.h`
- `Engine/Renderer/Private/RayTracing/RayTracingTlasBuilder.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingTlasBuilder.h`
- `Engine/Renderer/Private/RayTracing/RayTracingBlasCache.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingBlasCache.h`
- `Engine/Renderer/Private/Frame/RayTracingScene.cpp`
- `Engine/Renderer/Private/Frame/RayTracingScene.h`
- `Engine/Renderer/Private/Frame/RayTracingSceneFrameData.h`
- `Engine/Renderer/Private/RayTracing/RayTracingSceneDiagnostics.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingSceneDiagnostics.h`

RHI contract ownership:

- `Engine/RHI/Public/RayTracing/RhiRayTracingDesc.h`
- `Engine/RHI/Public/RayTracing/RhiRayTracingService.h`
- `Engine/RHI/Public/Commands/RenderCommandList.h`
- `Engine/RHI/Public/Core/RhiCapabilities.h`
- `Engine/RHI/Public/Shaders/Authoring/ShaderParameterStruct.h`
- `Engine/RHI/Private/Validation/RhiRayTracingValidation.cpp`

Vulkan backend ownership:

- `Engine/RHI/Private/Vulkan/RayTracing/VulkanRayTracingServices.cpp`
- `Engine/RHI/Private/Vulkan/Commands/VulkanRenderCommandList.cpp`
- `Engine/RHI/Private/Vulkan/Descriptors/VulkanDescriptorAllocator.cpp`
- `Engine/RHI/Private/Vulkan/Descriptors/VulkanDescriptorManager.cpp`

D3D12 backend ownership:

- `Engine/RHI/Private/D3D12/RayTracing/D3D12RayTracingServices.cpp`
- `Engine/RHI/Private/D3D12/Commands/D3D12RenderCommandList.cpp`

Validation and smoke ownership:

- `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp`
- `Engine/Application/Private/Validation/RhiSmokeCameraMotion.cpp`
- `Engine/Application/Private/Validation/RhiSmokeValidation.cpp`

## Existing Viewmode Files To Extend

- C++ enum: `Engine/Renderer/Public/Debug/RenderViewMode.h`
- HLSL constants: `Engine/Assets/Shaders/Debug/RenderViewModeConstants.hlsli`
- GBuffer instance coloring hook: `Engine/Assets/Shaders/Debug/InstanceView.hlsli`
- GBuffer usage: `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl`
- Deferred debug composite: `Engine/Assets/Shaders/Passes/Deferred/VisualizeBuffers.hlsl`
- Viewmode smoke naming and override path: `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp`

## Minimum Debug And Capture Artifacts

Later stages must produce these artifacts before the PTLAS implementation is considered review-ready:

| Artifact | Required content | Purpose |
|---|---|---|
| D3D12 classic TLAS lit screenshot | Same deterministic scene and camera used by all providers | Baseline image for D3D12. |
| Vulkan classic TLAS lit screenshot | Same deterministic scene and camera used by all providers | Baseline image for Vulkan. |
| Vulkan PTLAS lit screenshot | Same deterministic scene and camera | Proves PTLAS does not change final ray result. |
| D3D12 NVAPI PTLAS lit screenshot | Same deterministic scene and camera, when capability is present | Proves D3D12 provider parity. |
| `RayTracingPartitions` screenshot | Per-instance partition ID colors and grid/global partition indication | Explains partition policy. |
| `RayTracingPartitionUpdates` screenshot | Recently touched partitions and dirty counts | Explains sparse update behavior. |
| `RayTracingTopLevelMode` screenshot | Active provider and classic/PTLAS state | Shows capability negotiation result. |
| `RayTracingNativeOperations` screenshot or buffer capture | Native op count, op capacity, and op type categories | Explains backend work submitted. |
| `RayTracingGpuDrivenUpdates` screenshot | CPU pack, GPU logical dirty, GPU native pack, mismatch state | Explains update generation path. |
| `RayTracingProviderStatus` screenshot | Provider-selection reason | Explains why a provider is active. |
| Timing JSON or CSV | CPU planner, CPU pack, GPU dirty detection, GPU native pack, BLAS, TLAS/PTLAS, ray tracing pass | Supports performance claims. |
| Nsight Graphics capture | Named markers for provider query, logical dirty detection, native pack, PTLAS build/update | Supports NVIDIA review. |
| RenderDoc capture where supported | Descriptors, buffers, and frame graph resources | Supports general graphics review. |

## Stage 1 Acceptance Checklist

- [x] Baseline document exists.
- [x] Runtime code unchanged.
- [x] Vulkan, D3D12 NVAPI, and future public D3D12 DXR provider evidence recorded.
- [x] Current Sparkle classic TLAS owner files recorded.
- [x] Existing viewmode files to extend recorded.
- [x] Minimum screenshots, captures, and timing artifacts defined before implementation starts.

## Next Stage Entry Criteria

Stage 2 may start when:

- This baseline is committed or otherwise treated as the source of truth for PTLAS evidence.
- Runtime implementation still has not started.
- Provider-selection vocabulary is accepted: classic baseline, Vulkan NV PTLAS, D3D12 NVAPI PTLAS, future public D3D12 DXR PTLAS.
