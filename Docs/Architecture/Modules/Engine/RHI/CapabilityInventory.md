# RHI Capability Inventory

Status: capability snapshot; not release approval or executable backend evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; current `Engine/RHI` public contracts, common implementation, diagnostics, D3D12/Vulkan backends, CMake membership, and representative Renderer consumers inspected; evidence `S` only

Scope: backend-neutral and backend-specific device, queue, resource, descriptor, pipeline, command, ray-tracing, presentation, diagnostics, capture, memory, and interop capabilities

Owner: `Engine/RHI`

Architecture boundary: [Renderer and RHI Architecture Boundary](../../../Decisions/RendererRhiBoundary.md)

Evidence plan and release disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

Traceability: capability rows use durable `RHI-<family>-NN` identities; their primary proof destinations are listed in the [RHI capability-to-evidence map](../../../../Plans/CapabilityEvidence.md#rhi-capability-to-evidence-map).

Module routes: [RHI module index](README.md) and [RHI feature dossiers](Features/README.md)

Deeper routes: [cross-system backend/feature coverage](../../../CrossModule/GraphicsCoverageMatrix.md) and [producer-to-consumer execution traces](../../../CrossModule/FeatureExecutionTraces.md)

## Module Documentation

| Reader need | Owner |
| --- | --- |
| understand one RHI contract, its ownership/lifetime, or its feature-local completion requirements | [RHI Feature Dossiers](Features/README.md) |
| inspect exact source-state, backend coverage, limits, and evidence destinations | this capability inventory |
| understand Renderer policy above an RHI mechanism | [Renderer](../Renderer/README.md) |
| compare D3D12/Vulkan and execution-mode coverage | [Graphics Feature Coverage Matrix](../../../CrossModule/GraphicsCoverageMatrix.md) |
| inspect an end-to-end producer/consumer path | [Graphics Feature Execution Traces](../../../CrossModule/FeatureExecutionTraces.md) |

The capability rows below remain a compact ledger. Explanatory mechanisms and local `AC-*`/`FM-*`/`CHK-*` contracts belong to the linked feature dossiers and are not duplicated here.

## Module And Backend Shape

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `RHI-BACK-01` | Common public RHI | Implemented path | `SparkleRHICommon` owns backend-neutral resource, descriptor, pipeline, command, synchronization, presentation, ray-tracing, diagnostics, capture, and interop contracts. | `S` | Pending |
| `RHI-BACK-02` | Diagnostics implementation | Implemented path | `SparkleRHIDiagnostics` is a separate private dependency of backend targets rather than a Renderer service. | `S` | Pending |
| `RHI-BACK-03` | D3D12 backend | Capability-gated | `SparkleRHI_D3D12` is enabled by default on the current Windows build route. Device creation requests at least feature level 12_1. Runtime shader binary format is DXIL. | `S` | Pending |
| `RHI-BACK-04` | Vulkan backend | Capability-gated | `SparkleRHI_Vulkan` is enabled when the Vulkan SDK is found or required explicitly. Physical-device selection requires Vulkan 1.3. Runtime shader binary format is SPIR-V. | `S` | Pending |
| `RHI-BACK-05` | Backend selection | Implemented path | Root/module CMake selects compiled backends and a default backend; the RHI facade creates the requested backend and rejects unavailable selections. CMake assertions check common/backend source membership. | `S` | Pending |
| `RHI-BACK-06` | Backend parity | Partial | Both implementations cover the shared public contract, but this inventory produced no paired run, image comparison, native-validation record, or performance comparison. Optional/vendor paths are intentionally asymmetric. | `S` | Pending |

## Device Capabilities And Queues

`RhiCapabilities` is the runtime truth supplied by the selected adapter. Renderer features must query it instead of inferring support from the backend name.

| Capability ID | Capability | D3D12 | Vulkan | Important boundary | Evidence |
| --- | --- | --- | --- | --- | --- |
| `RHI-DEV-01` | API/version identity | D3D12 feature level recorded; minimum request 12_1 | Vulkan 1.3 required and recorded | Adapter/driver validation remains unrun. | `S` |
| `RHI-DEV-02` | Shader binary | DXIL | SPIR-V | Runtime consumes exact cooked target variants; source HLSL/Slang is not the runtime contract. | `S` |
| `RHI-DEV-03` | Queue kinds | Graphics, Compute, Copy | Graphics required; Compute and Copy reported from selected queue families | Capability records whether queues exist and whether they are independent. This does not prove useful async overlap. | `S` |
| `RHI-DEV-04` | Synchronization | Per-queue submissions/tokens and aggregate completion | Per-queue submissions/tokens; timeline semaphores and synchronization2 enabled when selected | Cross-queue scheduling exists at the contract level; deadlock/stress proof remains open. | `S` |
| `RHI-DEV-05` | Dynamic rendering | Native D3D12 render-target commands | Vulkan dynamic rendering feature | The Renderer does not expose legacy Vulkan render-pass ownership. | `S` |
| `RHI-DEV-06` | Descriptor indexing | Non-uniform sampled-image arrays and partially-bound arrays reported supported | Device features queried and required by eligible layouts | Renderer coverage is narrower than RHI capability; see Bindless Coverage. | `S` |
| `RHI-DEV-07` | Mesh shaders | Reported false | Reported false in the current capability surface | Mesh/task stage vocabulary must not be advertised as a working pipeline. | `S` |
| `RHI-DEV-08` | Task shaders | Reported false | Reported false in the current capability surface | No current graphics-pipeline binding path. | `S` |
| `RHI-DEV-09` | Adapter preference | D3D12 exposes high-performance/minimum-power selection | Vulkan scores physical devices during selection | These are not identical user contracts and need explicit release UX. | `S` |

## Resources, Views, Upload, And Memory

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `RHI-RES-01` | 2D textures | Implemented path | 2D and cube textures with mip count, array count, sample count, initial state, and optional clear value. There is no public 3D-texture kind in the inspected texture contract. | `S` | Pending |
| `RHI-RES-02` | Buffers | Implemented path | Generic, vertex, index, and structured buffers with size/stride/usage/state plus owned GPU address/allocation information. | `S` | Pending |
| `RHI-RES-03` | Resource views | Implemented path | Texture/buffer SRV and UAV; texture RTV and DSV; descriptor allocation/writes through backend-neutral handles. | `S` | Pending |
| `RHI-RES-04` | Upload | Implemented path | Uniform/constant data, arbitrary buffer data, and texture data upload routes. | `S` | Pending |
| `RHI-RES-05` | Readback | Implemented path | Buffer/texture readback capability is reported; asynchronous texture capture is used by the capture service. | `S` | Pending |
| `RHI-RES-06` | Persistent allocation | Implemented path | Backend-owned resources use D3D12 Memory Allocator or Vulkan Memory Allocator paths. Memory budget and allocation diagnostics are exposed. | `S` | Pending |
| `RHI-RES-07` | Transient allocation and aliasing | Implemented path | Transient memory blocks, placed/aliased textures and buffers, explicit alias commands, and resource transitions are available to the Renderer frame graph. | `S` | Pending |
| `RHI-RES-08` | Residency pressure | Implemented path | Delayed allocation tracking, budgets, and residency-pressure reporting exist in the diagnostics/memory surface. Policy quality under pressure is not measured. | `S` | Pending |
| `RHI-RES-09` | Samplers | Implemented path | Point/linear min-mag-mip filtering; wrap, clamp, and mirror address modes; anisotropy from x1 through x16. | `S` | Pending |

### Format Coverage

The public format vocabulary contains 23 concrete formats. Per-adapter support is queried rather than assumed.

| Capability ID | Family | Formats in the current public contract |
| --- | --- | --- |
| `RHI-FMT-01` | Float/color | `R32G32B32A32_Float`, `R16G16B16A16_Float`, `R32_Float`, `R16G16_Float` |
| `RHI-FMT-02` | 8-bit color | `R8G8B8A8_UNorm`, `R8G8B8A8_UNorm_Srgb`, `B8G8R8A8_UNorm`, `B8G8R8A8_UNorm_Srgb` |
| `RHI-FMT-03` | Depth/stencil | `D32_Float`, `D24_UNorm_S8_UInt` |
| `RHI-FMT-04` | BC color | BC1, BC2, BC3, and BC7 in UNORM and sRGB variants |
| `RHI-FMT-05` | BC scalar/vector | BC4 and BC5 in UNORM and SNORM variants |
| `RHI-FMT-06` | BC HDR | `BC6H_UF16` |

This list is a format contract, not evidence that every listed use—sampled, storage, render-target, depth, copy, filtering—passes on every release adapter. The evidence matrix must record the required usage bits for release content.

## Descriptor And Bindless Coverage

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `RHI-BIND-01` | Bindful descriptor sets/tables | Implemented path | Both backends expose explicit descriptor layouts, sets/tables, binding metadata, and sampler/resource writes. D3D12 reports one descriptor set in its modeled capability surface. | `S` | Pending |
| `RHI-BIND-02` | Fixed descriptor arrays | Implemented path | Layout bindings may declare arrays. Vulkan treats arrays larger than one as bindless-eligible and uses descriptor-indexing flags when device capabilities permit. | `S` | Pending |
| `RHI-BIND-03` | Non-uniform indexing | Capability-gated | Both backends report sampled-image non-uniform indexing; the shader compiler emits the Vulkan descriptor-indexing extension where required. | `S` | Pending |
| `RHI-BIND-04` | Partially-bound arrays | Capability-gated | Both capability paths expose this requirement. Renderer material-table selection requires it. | `S` | Pending |
| `RHI-BIND-05` | Renderer material texture table | Partial | The Renderer selects a fixed-capacity descriptor array only when both indexing capabilities exist and capacity reaches 4096 textures. Ray GBuffer, direct shadow, reference/path-traced, and ReSTIR-indirect shaders consume this table with non-uniform indices. | `S` | Pending |
| `RHI-BIND-06` | Raster-material coverage | Partial | Raster GBuffer uses a bindful per-material layout with eight texture roles: base color, normal, roughness, metallic, ambient occlusion, emissive, subsurface color, and subsurface strength. | `S` | Pending |
| `RHI-BIND-07` | Runtime-sized bindless | Not found | Renderer capability selection explicitly does not claim runtime-sized bindless. No engine-wide unbounded descriptor/resource model was found. | `S` | Pending |

The honest product statement at this snapshot is: **fixed-capacity bindless material-texture sampling is implemented for specific ray/path lighting paths; raster material sampling remains bindful; engine-wide bindless is not implemented.**

## Graphics And Compute Pipelines

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `RHI-PIPE-01` | Graphics pipeline | Implemented path | Vertex plus pixel shader pipeline; up to eight color attachments plus depth; sample count; input layout; raster, depth/stencil, and blend state. | `S` | Pending |
| `RHI-PIPE-02` | Primitive topology | Partial | Triangle list is the current public topology. No line, point, strip, patch, or meshlet dispatch topology was found. | `S` | Pending |
| `RHI-PIPE-03` | Vertex inputs | Partial | Position, texcoord, normal, and tangent semantics; float2/float3/float4 formats; up to four bindings and sixteen elements in the inspected contract. | `S` | Pending |
| `RHI-PIPE-04` | Index formats | Implemented path | 16-bit and 32-bit index buffers. | `S` | Pending |
| `RHI-PIPE-05` | Raster state | Implemented path | Solid/wireframe fill; none/front/back culling; front-face winding; depth clipping. | `S` | Pending |
| `RHI-PIPE-06` | Depth/stencil | Implemented path | Depth enable/write/compare and front/back stencil compare/operations/masks. | `S` | Pending |
| `RHI-PIPE-07` | Blending | Partial | Alpha-to-coverage and independent per-target state exist. Current blend operation is additive; factors include zero, one, source alpha, and inverse source alpha. | `S` | Pending |
| `RHI-PIPE-08` | Compute pipeline | Implemented path | One compute-shader program plus descriptor/push-constant contract and dispatch commands. | `S` | Pending |
| `RHI-PIPE-09` | Geometry/hull/domain stages | Vocabulary only | Shader-stage vocabulary exists, but the inspected graphics pipeline description binds only vertex and pixel programs. | `S` | Pending |
| `RHI-PIPE-10` | Mesh/task shaders | Vocabulary only | Capability fields exist and report false; no current graphics-pipeline route binds mesh/task programs. | `S` | Pending |

## Command Recording And Synchronization

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `RHI-CMD-01` | Recording leases | Implemented path | Device services issue graphics, compute, and copy recording leases and submit batches with explicit waits and completion tokens. | `S` | Pending |
| `RHI-CMD-02` | Raster commands | Implemented path | Render-target/depth setup, clear, viewport/scissor, vertex/index binding, instanced draw, and indexed-instanced draw. | `S` | Pending |
| `RHI-CMD-03` | Compute commands | Implemented path | Pipeline/resource/push-constant binding and dispatch. | `S` | Pending |
| `RHI-CMD-04` | Ray commands | Capability-gated | Acceleration-structure build/update commands, ray-query bindings, ray-tracing pipeline/shader-table binding, and `TraceRays`. | `S` | Pending |
| `RHI-CMD-05` | Copy commands | Implemented path | Buffer/texture copies and upload/readback transitions used by higher-level upload/capture paths. | `S` | Pending |
| `RHI-CMD-06` | Explicit state | Implemented path | Resource transitions, UAV barriers, and aliasing barriers are explicit RHI commands and are planned by the Renderer frame graph. | `S` | Pending |
| `RHI-CMD-07` | Diagnostic scopes | Implemented path | GPU event scopes and timestamp query operations are exposed through command recording. | `S` | Pending |

## Ray-Tracing Coverage

Ray tracing is not one boolean. The current contract separates acceleration structures, inline traversal, native pipeline dispatch, shader-table layout, classic TLAS, and partitioned TLAS.

| Capability ID | Capability | D3D12 path | Vulkan path | Current semantic limit | Evidence |
| --- | --- | --- | --- | --- | --- |
| `RHI-RT-01` | Bottom-level AS | DXR triangle geometry build | KHR acceleration-structure triangle geometry build | Public geometry description is one indexed triangle geometry with opaque control. Renderer static BLASes cache; deforming BLASes are rebuilt. | `S` |
| `RHI-RT-02` | BLAS update/refit | Prebuild vocabulary includes update sizing | Prebuild vocabulary includes update sizing | No public BLAS update/refit command or Renderer producer was found; do not advertise deforming-mesh refit. | `S` |
| `RHI-RT-03` | Classic TLAS | Build, allow-update, update/refit | Build, allow-update, update/refit | Instance ID, mask, shader-table contribution, flags, transform, and BLAS address are represented. | `S` |
| `RHI-RT-04` | Inline ray query | Requires DXR tier 1.1 capability | Requires Vulkan ray-query feature/extension | Renderer uses inline traversal for more effects than native pipeline traversal. | `S` |
| `RHI-RT-05` | Native RT pipeline | Requires DXR tier 1.0 | Requires KHR ray-tracing pipeline | Pipeline export/hit-group contracts, SBT creation, and `TraceRays` exist in both backends. No runtime proof was produced here. | `S` |
| `RHI-RT-06` | Shader table | Ray-generation, miss, hit, callable regions; local record bytes/signatures | Same backend-neutral regions mapped to Vulkan SBT | Renderer currently authors raygen/miss/hit records for two ray types; callable is contract capacity, not current effect coverage. | `S` |
| `RHI-RT-07` | Triangle hit groups | Closest-hit and any-hit exports | Closest-hit and any-hit exports | Procedural/intersection fields exist in the RHI contract, but no current Renderer procedural-geometry effect was found. | `S` |
| `RHI-RT-08` | Partitioned TLAS | NVIDIA NVAPI and public DXR RTAS-operation provider contracts | NVIDIA Vulkan extension provider | Vendor/API/capability gated. Current Renderer strategy disables instance updates and partition translation and emits at most one operation. | `S` |

### Current Renderer Traversal Consumers

| Capability ID | Effect | Inline ray query | Native RT pipeline | Notes |
| --- | --- | --- | --- | --- |
| `RHI-RTC-01` | Ray-traced GBuffer | Source path present | Source path present | Automatic mode prefers native pipeline when supported and falls back to inline; strict requested modes may reject graph creation. |
| `RHI-RTC-02` | Direct-shadow visibility | Source path present | Source path present | Shares the semantic shadow effect contract; user-facing parity remains unproven. |
| `RHI-RTC-03` | Reference path-traced direct/indirect | Source path present | Not found | Inline traversal only in this snapshot. |
| `RHI-RTC-04` | ReSTIR indirect temporal/spatial/resolve | Source path present | Not found | Inline traversal only in this snapshot. |

“Native ray tracing” must therefore be scoped to the GBuffer and direct-shadow effects, not the entire path/ReSTIR pipeline.

## Presentation

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `RHI-PRES-01` | Swapchain | Implemented path | Native window swapchain, two or three back buffers, one to three frames in flight constrained by back-buffer count, resize, acquire/current back buffer, and present. | `S` | Pending |
| `RHI-PRES-02` | VSync | Implemented path | Runtime setting reaches the presentation path. Tearing/present-mode behavior still needs backend/device evidence. | `S` | Pending |
| `RHI-PRES-03` | Frame pacing | Implemented path | D3D12 exposes a frame-latency waitable object; Vulkan exposes acquisition throttling in its capability surface. | `S` | Pending |
| `RHI-PRES-04` | Back-buffer commands | Implemented path | Back-buffer resource/RTV, format, viewport/scissor, transition, submit, and present are available to Renderer. | `S` | Pending |
| `RHI-PRES-05` | HDR presentation | Not found | The inspected public presentation/output contract did not establish HDR10, PQ, scRGB, display metadata, or HDR swapchain negotiation. | `S` | Pending |

## Diagnostics, Capture, And External Interop

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `RHI-DIAG-01` | Object naming and GPU events | Implemented path | Resource/object names and nested GPU diagnostic event scopes. | `S` | Pending |
| `RHI-DIAG-02` | Timestamp queries | Implemented path | Per-command timestamp operations used for pass/frame timing diagnostics. | `S` | Pending |
| `RHI-DIAG-03` | Validation messages | Capability-gated | Backend debug/validation messages are captured when the relevant layers are enabled and installed. | `S` | Pending |
| `RHI-DIAG-04` | D3D12 crash diagnostics | Capability-gated | DRED settings/reporting are present in the D3D12 path. Equivalent cross-backend crash diagnostics were not found. | `S` | Pending |
| `RHI-DIAG-05` | Live-object reporting | Capability-gated | Backend diagnostic surface includes live-object reporting where supported. | `S` | Pending |
| `RHI-DIAG-06` | Texture capture | Implemented path | Asynchronous texture readback flows to bitmap/capture result handling. Supported formats, colorspace correctness, and failure UX need runtime evidence. | `S` | Pending |
| `RHI-DIAG-07` | External provider handles | Partial | A narrow native-resource/command/device interop surface supports optional providers. D3D12 includes Streamline interposer/manual routes; Vulkan external evaluation is not an equivalent active path. | `S` | Pending |
| `RHI-DIAG-08` | ImGui backend | Implemented path | Backend-specific ImGui rendering integration exists for D3D12 and Vulkan. | `S` | Pending |

## Explicit Non-Claims And Shipping Risks

- No backend build, launch, resize loop, device removal, native validation, or capture was run for this snapshot.
- D3D12 feature level 12_1 and Vulkan 1.3 are source-configured requirements, not yet a published and tested minimum hardware/driver matrix.
- Equal public RHI shape does not prove equal pixels, synchronization behavior, error reporting, memory behavior, or performance.
- Fixed-capacity material descriptors are not an engine-wide bindless resource model.
- Native RT pipeline support does not cover every ray-using Renderer effect.
- The RHI has procedural/callable/advanced stage vocabulary beyond current Renderer production paths. Such vocabulary is not a shippable feature.
- No current HDR presentation contract was found.
- Rich partitioned-TLAS operation vocabulary exceeds the current Renderer strategy. Advertise only the actually selected and exercised subset.

## Primary Source Routes

- Public capability and service contracts: `Engine/RHI/Public/Core`, `Engine/RHI/Public/Resources`, `Engine/RHI/Public/Descriptors`, `Engine/RHI/Public/Pipeline`, `Engine/RHI/Public/Commands`, `Engine/RHI/Public/RayTracing`, `Engine/RHI/Public/Presentation`, `Engine/RHI/Public/Diagnostics`, and `Engine/RHI/Public/Interop`.
- Backend-neutral implementation and device facade: the non-backend subdirectories of `Engine/RHI/Private`, especially `Core`, `Device`, `Resources`, `Descriptors`, `Pipeline`, `Commands`, `Capture`, `Diagnostics`, and `Validation`.
- D3D12 implementation: `Engine/RHI/Private/D3D12`.
- Vulkan implementation: `Engine/RHI/Private/Vulkan`.
- Build membership and backend switches: `Engine/RHI/CMakeLists.txt` and root CMake options/assertions.
- Material descriptor coverage: `Engine/Renderer/Private/Scene/Materials`, `Engine/Renderer/Private/Passes/GBuffer/GBufferMeshBatchDrawer.cpp`, and `Engine/Assets/Shaders/Material/MaterialTextureTable.hlsli`.
- Effect traversal coverage: `Engine/Renderer/Private/Passes/GBuffer`, `Engine/Renderer/Private/Passes/Lighting`, and `Engine/Renderer/Private/RayTracing`.
