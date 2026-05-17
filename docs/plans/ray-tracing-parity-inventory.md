# Ray Tracing Parity Inventory

This inventory records the current Sparkle ray tracing surface for Phase 18 of the Vulkan multi-backend roadmap. The decision is staged parity: Vulkan can become first-class for the raster FrameGraph path before Vulkan ray tracing is implemented, but ray tracing remains a tracked backend gap rather than a silent omission.

## Current Usage

| Area | Current state | Parity status |
| --- | --- | --- |
| Showcase project | No active runtime ray tracing pass usage was found. Bistro asset notes mention raytracing-friendly emissive surfaces, but no Showcase code currently builds or dispatches ray tracing workloads. | Future |
| Renderer mesh data | `GPUMesh` can expose a neutral triangle geometry description from uploaded vertex/index buffers. | Parity-ready data surface |
| Renderer command context | Renderer forwards neutral BLAS/TLAS build commands to `RenderCommandList`; it does not name DXR, D3D12, Vulkan, or native AS structs. | Backend-neutral |
| Public RHI device | Public methods expose capabilities, BLAS/TLAS prebuild sizes, scratch/AS/instance buffer creation, and resource GPU addresses. | Backend-neutral, D3D12 implemented |
| Public RHI command list | Public commands expose BLAS/TLAS build intent in neutral terms. | Backend-neutral, D3D12 implemented |
| Public resource views | `AccelerationStructureShaderResource` is represented as a logical view from a GPU address. | Backend-neutral, binding-ready |
| Shader authoring and package data | Ray generation, miss, closest hit, any hit, intersection, callable, hit group, payload, attribute, recursion, and acceleration-structure binding metadata exist in cooked shader records. | Tooling-ready, runtime RT pipeline deferred |

## Current Backend Surface

| Concept | D3D12 implementation | Vulkan implementation now | Vulkan KHR target |
| --- | --- | --- | --- |
| Capability query | D3D12 reports `RhiRayTracingCapabilities` from device support. | Explicitly returns unsupported capabilities. | Query `VkPhysicalDeviceAccelerationStructureFeaturesKHR`, `VkPhysicalDeviceRayTracingPipelineFeaturesKHR`, and `VkPhysicalDeviceRayQueryFeaturesKHR`. |
| BLAS prebuild info | `GetRaytracingAccelerationStructurePrebuildInfo` with triangle geometry. | Returns empty prebuild info. | `vkGetAccelerationStructureBuildSizesKHR` with `VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR`. |
| TLAS prebuild info | `GetRaytracingAccelerationStructurePrebuildInfo` with instance count. | Returns empty prebuild info. | `vkGetAccelerationStructureBuildSizesKHR` with `VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR`. |
| Scratch buffer | D3D12MA device-local UAV buffer in `RhiMemoryCategory::RayTracing`. | Deferred and fails explicitly if called. | VMA buffer with `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT`, device-local residency, `RayTracing` category. |
| Acceleration structure backing buffer | D3D12MA device-local UAV buffer in `RAYTRACING_ACCELERATION_STRUCTURE` state. | Deferred and fails explicitly if called. | VMA buffer with `VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT`, then `vkCreateAccelerationStructureKHR`. |
| Instance buffer | D3D12 upload buffer filled with `D3D12_RAYTRACING_INSTANCE_DESC`. | Deferred and fails explicitly if called. | VMA host-upload buffer filled with `VkAccelerationStructureInstanceKHR`. |
| BLAS build command | `BuildRaytracingAccelerationStructure` with triangle geometry. | Fails explicitly; no silent no-op. | `vkCmdBuildAccelerationStructuresKHR` with `VkAccelerationStructureGeometryKHR` triangles. |
| TLAS build command | `BuildRaytracingAccelerationStructure` with instance descriptors. | Fails explicitly; no silent no-op. | `vkCmdBuildAccelerationStructuresKHR` with instance geometry. |
| AS shader resource view | D3D12 descriptor path can consume a GPU address. | Descriptor model has the neutral view kind, but Vulkan descriptor writes for AS remain deferred. | `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` writes through binding sets. |
| Ray tracing pipeline/SBT | Shader package metadata exists, but no complete runtime pipeline/SBT surface was found in the current RHI. | Deferred. | Future neutral ray tracing pipeline state and shader binding table model before backend implementation. |

## Concept Map

| Sparkle concept | D3D12/DXR concept | Vulkan KHR concept | Notes |
| --- | --- | --- | --- |
| `RhiRayTracingGeometryDesc` | `D3D12_RAYTRACING_GEOMETRY_DESC` triangles | `VkAccelerationStructureGeometryKHR` triangles | Current public geometry assumes triangle meshes with R32G32B32 float positions and an index buffer. |
| `RhiRayTracingInstanceDesc` | `D3D12_RAYTRACING_INSTANCE_DESC` | `VkAccelerationStructureInstanceKHR` | Public fields are neutral enough for transforms, IDs, masks, hit-group offset, and referenced BLAS address. |
| BLAS prebuild info | `D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO` | `VkAccelerationStructureBuildSizesInfoKHR` | Public size structure maps cleanly to both APIs. |
| Scratch buffer | UAV buffer | storage/device-address buffer | Keep in `RhiMemoryCategory::RayTracing`; Vulkan uses VMA for backing memory. |
| AS result buffer | resource in AS state | acceleration structure storage buffer plus `VkAccelerationStructureKHR` object | Vulkan likely needs a backend-private AS object record in addition to the public resource handle. |
| AS GPU address | `D3D12_GPU_VIRTUAL_ADDRESS` | `vkGetAccelerationStructureDeviceAddressKHR` | Public `RhiGpuVirtualAddress` is acceptable as an opaque address value. |
| AS SRV binding | D3D12 SRV descriptor with raytracing AS location | `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` | Existing logical view kind is a good public contract. |

## Staged Decision

Raster Vulkan parity can be treated as first-class before ray tracing parity because the current Showcase path does not actively depend on ray tracing runtime execution. Ray tracing is classified as `D3D12OnlyTemporary` until a dedicated Vulkan KHR ray tracing phase implements capabilities, VMA-backed AS buffers, BLAS/TLAS build commands, AS descriptor writes, and any required neutral ray tracing pipeline/SBT API.

First-class Vulkan claims must use this wording: Sparkle has first-class Vulkan raster/FrameGraph parity; ray tracing is an explicit staged parity item, not part of the current first-class raster milestone.

## Deferred Vulkan Ray Tracing Requirements

1. Enable and validate `VK_KHR_acceleration_structure`, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_deferred_host_operations`, `VK_KHR_buffer_device_address`, and optionally `VK_KHR_ray_query` when inline ray query support is needed.
2. Add backend-private Vulkan AS records that keep `VkAccelerationStructureKHR`, the VMA-backed storage buffer, device address, type, size, and debug name together.
3. Create scratch and AS storage buffers through `VulkanGpuMemoryAllocator` using `RhiMemoryCategory::RayTracing` and VMA device-local memory.
4. Convert `RhiRayTracingInstanceDesc` into `VkAccelerationStructureInstanceKHR` inside Vulkan backend code only.
5. Implement `vkGetAccelerationStructureBuildSizesKHR`, `vkCmdBuildAccelerationStructuresKHR`, and required synchronization through the neutral resource-state/barrier model.
6. Add AS descriptor writes to Vulkan descriptor sets through `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` without exposing descriptor-set types to Renderer.
7. Add a neutral ray tracing pipeline and shader binding table plan before implementing ray generation dispatch, because the current public RHI only covers AS build and AS binding metadata.

## Validation Notes

Source gates should keep the current boundary honest:

1. Public RHI and Renderer must not include DXR, D3D12 ray tracing structs, Vulkan KHR structs, or VMA types.
2. Vulkan ray tracing methods must remain explicitly unsupported until real KHR support lands.
3. D3D12 ray tracing implementation must stay backend-private and allocator-backed through D3D12MA.
4. Vulkan AS backing buffers must use VMA when implemented.