# SparkleEngine RHI Contract

This document describes the current RHI contract in SparkleEngine based on the public `Engine/RHI/Public` surface and representative D3D12/Vulkan backend implementation files. The goal is to make ownership, lifetime, backend responsibilities, and extension rules clear before more rendering features arrive.

## Purpose And Non-Goals

Purpose:

- Explain what the public RHI surface promises today.
- Show which responsibilities belong to `RenderDeviceServices`, `RenderHardwareInterface`, and the service interfaces beneath it.
- Keep backend-native details owned by backend-private D3D12/Vulkan code.
- Make extension work for backends and native interop easier to reason about.

Non-goals:

- This is not a renderer frame graph document.
- This is not a provider integration contract.
- This does not invent guarantees that are not visible in source.
- This does not claim D3D12/Vulkan feature parity beyond what the code clearly exposes.

Notation used here:

- `Current behavior`: visible in source.
- `Needs source confirmation`: likely true, but not stated strongly enough in the inspected files to treat as contract yet.
- `Planned contract`: desirable contract direction already implied by the architecture docs, but not yet fully established in code/docs.

## Public RHI Surface Overview

The top-level public entrypoints are:

- `RenderDeviceServices`
- `RenderHardwareInterface`
- `RhiCommandSubmissionService`
- `RhiResourceService`
- `RhiUploadService`
- `RhiDescriptorService`
- `RhiPipelineService`
- `RhiPresentationService`
- `RhiDiagnosticsService`
- `RhiInteropService`
- `RhiCaptureService`
- `RhiRayTracingService`
- `RhiClassicTlasService`
- `RhiPartitionedTlasService`
- `RhiValidation`

Current behavior:

- `RenderDeviceServices` is the public device/backend host object created from `Timer` and `Window`.
- `RenderDeviceServices` also implements `RhiCommandSubmissionService`.
- `RenderHardwareInterface` is the service locator and backend capability surface beneath `RenderDeviceServices`.
- Individual concerns are split into service interfaces instead of one monolithic API.

Key public capability carriers:

- `RhiCapabilities`
- `RhiMemoryUsageSnapshot`
- `RhiNativeDeviceQueueInterop`
- `NativeTextureViewInfo`
- `ResourceState`

## Backend Ownership Model

Current behavior:

- `RHI` owns backend abstraction and backend-private implementations.
- `RenderDeviceServices::Create` selects the backend and constructs backend-specific device services.
- D3D12 backend creation happens through `CreateD3D12RenderDeviceServices`.
- Vulkan backend creation happens through `CreateVulkanRenderDeviceServices`.
- `RenderHardwareInterface` is implemented by `D3D12RenderHardwareInterface` and `VulkanRenderHardwareInterface`.

Current ownership split:

- `RenderDeviceServices`
  - owns backend selection and backend service lifetime
  - exposes frame submission/orchestration entrypoints
- backend-private `*RenderDeviceServices`
  - own backend bootstrap order
  - construct backend root objects such as device, swapchain, command context/allocator structures, memory allocator, and hardware interface
- backend-private `*RenderHardwareInterface`
  - own service objects for resources, descriptors, pipelines, interop, capture, diagnostics, presentation, and ray tracing

Explicit architectural rule:

- Renderer code must consume this layer through backend-neutral interfaces.
- Backend-native D3D12/Vulkan types remain in `Engine/RHI/Private/D3D12` and `Engine/RHI/Private/Vulkan`.

## Device Lifecycle

Current behavior:

- Public creation starts at `RenderDeviceServices::Create(Timer&, Window&)` or `RenderDeviceServices::Create(Timer&, Window&, RhiBackendSelection)`.
- Backend creation order is explicit in backend-private device services:
  - D3D12: device (`D3D12Rhi`), descriptor heaps, swapchain, frame resources, constant buffer manager, hardware interface, sampler library
  - Vulkan: device (`VulkanRhi`), memory allocator, swapchain, command context, hardware interface

Current behavior during destruction:

- D3D12 backend flushes before tearing down services and reports live objects in debugger builds.
- Vulkan backend waits for idle before tearing down hardware interface, command context, swapchain, memory allocator, and device.

Needs source confirmation:

- Whether all service destruction order dependencies are formally documented anywhere beyond constructor/destructor order in backend-private code.

Planned contract:

- Device lifecycle should eventually be diagrammed with explicit startup/shutdown invariants and failure categories.

## Adapter And Backend Selection

Current behavior:

- `RhiBackendSelection` carries `ERhiBackendApi`.
- `ResolveDefaultRhiBackendSelection()` resolves the default backend from:
  - build default (`SPARKLE_RHI_DEFAULT_BACKEND_*`)
  - optional environment variable `SPARKLE_RHI_BACKEND`
  - optional command-line overrides:
    - `--renderer`
    - `--rhi`
    - `--graphics-api`
    - plus `=value` variants

Current backend availability model:

- D3D12 backend is controlled by `SPARKLE_RHI_WITH_D3D12`.
- Vulkan backend is controlled by `SPARKLE_RHI_WITH_VULKAN` and Vulkan SDK discovery.
- If Vulkan is enabled but the SDK is missing, configure either fails or disables Vulkan depending on build settings and selected default backend.

Current adapter identity model:

- `RhiCapabilities::ExternalFeatureInterop.Adapter` exposes adapter name, driver description, vendor ID, device ID, and native LUID bytes where available.
- D3D12 adapter identity is derived from DXGI.
- Vulkan adapter identity is derived from `VulkanAdapterInfo`.

## Resource Lifetime Model

Current behavior:

- Resource creation is owned by `RhiResourceService`.
- Public resource ownership is expressed through opaque handles:
  - `RhiOwnedResourceHandle`
  - `RhiOwnedMemoryBlockHandle`
  - `RhiResourceViewHandle`
- Public release is explicit:
  - `ReleaseOwnedResource`
  - `ReleaseTransientMemoryBlock`
  - `ReleaseResourceView`

Current resource categories:

- textures
- buffers
- vertex buffers
- structured buffers
- index buffers
- transient memory blocks
- aliasing texture resources
- aliasing buffer resources

Current behavior:

- Public resource creation requires:
  - initial `ResourceState`
  - `RhiMemoryCategory`
  - `RhiMemoryResidencyClass`
  - debug name

Current memory-oriented resource patterns:

- explicit transient memory blocks
- explicit aliasing resource creation against memory blocks
- allocation info queries for textures and buffers

Needs source confirmation:

- Whether `RhiOwnedResourceHandle` destruction is always immediate on D3D12 or may be deferred through backend-private lifetime handling in some paths.

Source-backed distinction:

- Vulkan allocator explicitly queues pending releases and drains them by completed fence value.
- D3D12 lifetime is explicit through owned-handle release APIs, but deferred destruction policy is less obvious from the sampled public surface alone.

## Resource State And Barrier Model

Current behavior:

- Public backend-neutral resource states are defined by `ResourceState`.
- `RenderCommandList` owns explicit state transition and UAV barrier commands:
  - `TransitionResource(resource, before, after)`
  - `UnorderedAccessBarrier(resource)`
- `RenderCommandList` also exposes `AliasResource(before, after)` for aliasing transitions.

Contract implication:

- Resource state ownership is explicit at command-list call sites, not hidden behind a fully automatic barrier model.

Current behavior:

- D3D12 command list implements explicit transition and UAV barrier methods.
- Vulkan command list implements explicit transition and UAV barrier methods.
- Presentation paths transition the back buffer explicitly between `Present` and `RenderTarget`.

Needs source confirmation:

- Full subresource barrier granularity policy.
- Whether additional implicit transitions exist in specific helper paths beyond those seen in the sampled files.

Planned contract:

- A future renderer/frame-graph document should define which layer decides transition timing for transient and persistent render resources.

## Command List, Queue, Submit, And Fence Model

Current behavior:

- `RhiCommandSubmissionService` defines:
  - `WaitForIdle`
  - `Flush`
  - `ResizeSwapChain`
  - `BeginFrame`
  - `GetCurrentGraphicsCommandList`
  - `GetGraphicsCommandList(frameIndex)`
  - `SubmitFrame`
  - `AdvanceFrameInFlight`
  - `CloseExecuteAndFlushCurrentFrame`

Current D3D12 behavior:

- `D3D12Rhi` owns:
  - command queue
  - per-frame command allocators
  - per-frame command lists
  - fence
  - fence event
  - current frame index
- `D3D12RenderDeviceServices::BeginFrame`:
  - sets frame index
  - begins frame resource management
  - waits for GPU for that frame
  - resets allocator
  - resets command list
- `SubmitFrame`:
  - closes command list
  - executes command list
  - signals fence
  - ends frame resource tracking
  - presents swapchain

Current Vulkan behavior:

- `VulkanRenderDeviceServices` owns frame index and `VulkanCommandContext`.
- `BeginFrame`:
  - sets frame index
  - begins command context frame
  - resets transient frame resources
  - acquires back buffer
  - cancels frame and rebuilds back-buffer views if acquire fails
- `SubmitFrame`:
  - submits command context frame with semaphores
  - presents swapchain
  - rebuilds back-buffer views on swapchain-present rebuild request

Current queue model in capabilities:

- Both D3D12 and Vulkan currently report:
  - graphics queue supported
  - compute queue unsupported
  - copy queue unsupported

Important constraint:

- That is a current contract of exposed engine behavior, not a statement about underlying API capability in general.

## Descriptor Ownership Model

Current behavior:

- Descriptor allocation and descriptor-table allocation are owned by `RhiDescriptorService`.
- Public APIs include:
  - `AllocateDescriptor`
  - `ReleaseDescriptor`
  - `AllocateDescriptorTable`
  - `GetDescriptorTableCpuHandle`
  - `ReleaseDescriptorTable`
  - `AllocateShaderResourceDescriptor`
  - `ReleaseShaderResourceDescriptor`
  - `CreateResourceView`
  - `ReleaseResourceView`
  - `GetResourceViewCpuHandle`
  - `GetResourceViewGpuHandle`
  - `GetSharedSamplerBinding`
  - `GetNativeTextureViewInfo`

Current descriptor model by backend:

- D3D12 reports `DescriptorTables`.
- Vulkan reports `DescriptorSets`.

Interpretation:

- The public API intentionally normalizes descriptor usage even though backend implementation models differ.
- The capability surface is honest about the underlying descriptor model through `RhiCapabilities::DescriptorModel`.

Needs source confirmation:

- Exact reuse/recycling guarantees for descriptor handles across frames.

Planned contract:

- Descriptor lifetime and residency behavior should eventually be documented together with frame-graph/persistent-resource rules.

## Pipeline And Shader Binding Model

Current behavior:

- `RhiPipelineService` owns:
  - binding layout creation
  - graphics pipeline state creation
  - compute pipeline state creation

Current command-list binding model:

- explicit graphics and compute binding layouts
- explicit constant buffer binding
- explicit shader resource binding
- explicit unordered access binding
- explicit descriptor-table binding
- explicit push constants

Current shader binary expectations:

- D3D12 requires `CookedShaderBinaryFormat::Dxil`.
- Vulkan requires `CookedShaderBinaryFormat::SpirV`.

Current behavior:

- Binding layout and pipeline-state compilation stay in backend-private pipeline services.
- Command-list usage consumes backend-neutral `RenderBindingLayout` and `RenderPipelineState`.

Needs source confirmation:

- Full cache behavior for pipeline state reuse and invalidation.

## Memory Allocation And Budget Model

Current behavior:

- Public memory categories and residency classes are explicitly modeled:
  - `RhiMemoryCategory`
  - `RhiMemoryResidencyClass`
- Public memory diagnostics snapshot is exposed through `RenderMemoryDiagnostics`.
- Snapshot fields include:
  - total used bytes
  - total allocated bytes
  - total budget bytes
  - API usage bytes
  - committed usage bytes
  - placed usage bytes
  - transient usage bytes
  - delayed destruction bytes
  - delayed destruction allocation count
  - per-category stats
  - allocation list

Allocator ownership by backend:

- D3D12 backend uses `D3D12GpuMemoryAllocator` and reports `ERhiMemoryAllocatorBackend::D3D12Managed`.
- Vulkan backend uses `VulkanGpuMemoryAllocator` and reports `ERhiMemoryAllocatorBackend::VulkanManaged`.
- Build configuration fetches:
  - D3D12 Memory Allocator (`D3D12MA`)
  - Vulkan Memory Allocator (`VMA`)

Current diagnostics hooks:

- both allocators expose:
  - `SupportsBudgetQueries`
  - `SupportsJsonDump`
  - `CreateMemoryUsageSnapshot`
  - `WriteAllocatorJsonDump`

Current behavior visible in source:

- D3D12 allocator creates textures, buffers, transient heaps, and aliasing resources.
- Vulkan allocator creates buffers, images, transient memory blocks, aliasing resources, supports data writes, and explicitly queues/destroys pending releases based on fence completion.

Needs source confirmation:

- Whether D3D12 delayed-destruction behavior is symmetric with Vulkan at the allocator level or handled differently.

## Ray Tracing Service Ownership

Current behavior:

- `RhiRayTracingService` owns the primary backend-neutral ray tracing surface.
- It exposes:
  - ray tracing capabilities
  - BLAS/TLAS prebuild info
  - scratch buffer creation
  - acceleration structure buffer creation
  - instance buffer creation
  - classic TLAS sub-service
  - partitioned TLAS sub-service

Current D3D12 behavior:

- `D3D12RayTracingServices` owns:
  - `D3D12ClassicTlasServices`
  - `D3D12PartitionedTlasServices`
  - NVAPI-backed partitioned TLAS provider integration through `D3D12NvapiRayTracingProvider`

Current Vulkan behavior:

- `VulkanRayTracingServices` owns:
  - `VulkanClassicTlasServices`
  - `VulkanPartitionedTlasServices`

Important architecture point:

- Renderer must use backend-neutral RHI ray tracing services and PTLAS structs rather than native backend PTLAS identifiers.

Needs source confirmation:

- Full feature-level parity of partitioned TLAS operations between D3D12 and Vulkan beyond the visible service APIs.

## UI And Presentation Ownership

Current behavior:

- UI integration is owned by `RhiImGuiRenderer`.
- Presentation ownership is owned by `RhiPresentationService`.

Presentation service responsibilities:

- expose back-buffer viewport and scissor
- expose back-buffer RTV handle
- expose back-buffer native resource
- resolve ImGui texture IDs
- begin/end present render pass
- begin present overlay pass
- expose present color format

Current backend realization:

- D3D12 presentation is implemented by `D3D12PresentationService`.
- Vulkan presentation is implemented by `VulkanPresentationService`.

Contract implication:

- Presentation is part of RHI ownership, not renderer ownership.

## Diagnostics And Validation Ownership

Current behavior:

- `RhiDiagnosticsService` exposes `RenderDiagnostics`.
- `RenderDiagnostics` splits diagnostics into:
  - object diagnostics
  - timing diagnostics
  - message diagnostics
  - failure diagnostics
  - memory diagnostics

Current diagnostic capability surface includes:

- object names
- GPU events
- timestamp queries
- debug messages
- live object reports
- crash diagnostics
- memory diagnostics
- memory budget queries
- allocator JSON dump support

Current backend behavior visible in source:

- D3D12 reports timestamp query support through its capabilities and diagnostics implementation.
- Vulkan currently reports `SupportsTimestampQueries = false` in `VulkanRenderHardwareInterface::BuildCapabilities`.
- Both backends expose memory diagnostics through backend-private render diagnostics objects.
- `RhiValidation` provides public validation helpers for binding sets, textures, and ray tracing descriptors/addresses/sizes.

Important ownership line:

- application/editor validation should consume these RHI-facing validation/diagnostic surfaces rather than growing backend-native dependencies.

## Native Handle And Interop Policy

Current behavior:

- Native handles are explicit opaque public types:
  - `NativeGraphicsDeviceHandle`
  - `NativeGraphicsQueueHandle`
  - `NativeGraphicsCommandListHandle`
  - `NativeResourceHandle`
  - `NativeTextureViewHandle`
- Native interop is owned by `RhiInteropService`.

Current interop requests are tagged by consumer:

- `Validation`
- `UpscalerProvider`
- `RendererFrameGraph`
- `PresentationBridge`

Current interop service responsibilities:

- expose native device/queue interop bundle
- expose native device handle
- expose native graphics queue handle
- allow presentation interface upgrade callback
- expose native texture view info for a resource view plus explicit `ResourceState`

Current external interop capability model:

- D3D12 reports `BridgeKind = D3D12NativeDevice`
- Vulkan reports `BridgeKind = VulkanManualFunctionPointers`
- both report `SupportsExplicitResourceStates = true`
- both report `ExposesNativeResources = true`
- both compute `SupportsExternalProviderEvaluation`
- both compute `SupportsRuntimeProviderChecks`

Current nuance:

- Vulkan capability data also carries instance, physical-device, device, queue, and queue-family handles for manual-function-pointer style provider integration.
- Vulkan currently reports `VulkanInterposerRequired = false`.

Contract rule:

- Native interop is permitted through `RhiInteropService` and capability queries, not by bypassing the RHI boundary from renderer code.

## Backend Parity Matrix

This matrix only captures source-backed observations from the inspected files.

| Area | D3D12 | Vulkan | Notes |
| --- | --- | --- | --- |
| Public `RenderHardwareInterface` implementation | Yes | Yes | Both have backend-private concrete implementations. |
| Public backend selection support | Yes | Yes | Selection resolved through `RhiBackendSelection` and backend factory. |
| Default shader binary format | `Dxil` | `SpirV` | Source-backed in backend hardware interfaces. |
| Descriptor model | `DescriptorTables` | `DescriptorSets` | Reported through `RhiCapabilities`. |
| Graphics queue support | Yes | Yes | Reported in capabilities. |
| Compute queue support | No | No | Current exposed engine behavior, not API claim. |
| Copy queue support | No | No | Current exposed engine behavior, not API claim. |
| Buffer upload support | Yes | Yes | Reported in capabilities. |
| Texture upload support | Yes | Yes | Reported in capabilities. |
| Readback support | Yes | Yes | Reported in capabilities. |
| Timestamp query support | Yes | No | Vulkan currently reports false. |
| Presentation support | Yes | Yes | Both report present support when swapchain format is valid. |
| Memory allocator backend | `D3D12Managed` | `VulkanManaged` | Backed by D3D12MA and VMA integrations. |
| Memory budget query surface | Yes | Yes | Both allocators expose support query and snapshot path. |
| Allocator JSON dump surface | Yes | Yes | Both allocators expose JSON dump API. |
| Native device/queue interop | Yes | Yes | Through `RhiInteropService`. |
| Explicit resource-state interop capability | Yes | Yes | Reported in external interop capabilities. |
| Native texture-view info | Partial | Yes | Both expose API; Vulkan implementation clearly returns populated native view info. D3D12 public implementation path needs deeper source confirmation for richness of returned view info. |
| Classic TLAS service | Yes | Yes | Both expose backend-private implementations. |
| Partitioned TLAS service | Yes | Yes | Both expose backend-private implementations. |
| Partitioned TLAS provider dependence | NVAPI-backed provider present | Vulkan-native service present | Parity of full behavior needs deeper confirmation. |
| ImGui renderer | Yes | Yes | Both backends expose `RhiImGuiRenderer`. |
| Capture service | Yes | Yes | Both backends expose `RhiCaptureService`. |
| Crash diagnostics | Present in capability surface | Present in capability surface | Exact parity of implementation depth needs deeper confirmation. |
| Live-object reporting | Present in D3D12 source | Needs source confirmation | D3D12 clearly reports debugger/live-object support; Vulkan equivalent support should be treated as capability-surface-dependent until confirmed more deeply. |

## New Backend Checklist

- Implement a backend-private `RenderHardwareInterface`.
- Implement backend-private device/bootstrap ownership equivalent to `*RenderDeviceServices`.
- Expose truthful `RhiCapabilities`:
  - shader binary format
  - descriptor model
  - queue support
  - present support
  - ray tracing support
  - memory allocator backend
  - external interop capabilities
- Implement:
  - `RhiResourceService`
  - `RhiDescriptorService`
  - `RhiPipelineService`
  - `RhiUploadService`
  - `RhiPresentationService`
  - `RhiDiagnosticsService`
  - `RhiInteropService`
  - `RhiCaptureService`
  - `RhiRayTracingService`
- Keep backend-native code inside `Engine/RHI/Private/<Backend>`.
- Do not introduce renderer-private dependencies.
- Extend the backend parity table.
- Extend validation and diagnostics reporting honestly, even if some capabilities are unsupported.

## New Native Interop Checklist

- Confirm the use case cannot stay backend-neutral.
- Route native access through `RhiInteropService` or capability data, not direct renderer-native includes.
- Tag the consumer through `RhiNativeInteropRequest`.
- Expose truthful capability data in `RhiCapabilities::ExternalFeatureInterop`.
- Keep backend-native handle assembly inside backend-private code.
- If renderer/provider code needs a narrow exception, document it through the boundary-rule/ADR process instead of broadening renderer permissions.
- Add diagnostics or failure-state reporting if the interop path can be unavailable at runtime.
- Document whether the interop path requires:
  - explicit resource states
  - native texture view info
  - presentation interface upgrade
  - queue-family/command-list visibility

## Known Gaps

- Resource lifetime/destruction semantics are clearer on Vulkan than on D3D12 from the currently inspected files; D3D12 deferred-destruction policy needs deeper source confirmation.
- Subresource/barrier granularity policy is not yet documented as a stable contract.
- Pipeline cache and PSO reuse behavior are not described here because the inspected public surface does not establish a strong contract for them yet.
- The engine currently reports no compute/copy queue support in capabilities even though underlying APIs can support more; future expansion should treat this as an exposed-engine-policy decision, not a hardware statement.
- Timestamp-query parity is not there today: D3D12 reports support, Vulkan currently reports none.
- Native texture-view interop richness is more obvious on Vulkan than on D3D12 from the sampled implementation files.
- The RHI contract still needs companion documents:
  - renderer frame graph contract
  - provider interop contract
  - validation matrix
  - performance diagnostics plan

