# Renderer and RHI Boundary

Status: canonical implementation policy  
Reference revisions: NVIDIA NVRHI `8e8c36e37558acec333204619b95d9d2fcdc4a79`; NVIDIA Donut `bfdebdd7dd5455c503b2737a1967a4ef651c145b`

## Dependency Direction

The dependency direction is one way:

```text
Application / Editor
        |
     Renderer  ---- dedicated external-provider adapters
        |
   public RHI contracts
        |
   RHI common implementation
      /     \
  D3D12    Vulkan
```

- Renderer never includes RHI private headers or native D3D12/Vulkan types.
- RHI never includes Renderer code or owns render-pass, frame-graph, scene, material, upscaling, or ray-reconstruction policy.
- Common RHI code never includes either backend implementation. Backend selection may name an API, but native types and calls stay in `Private/D3D12` or `Private/Vulkan`.
- D3D12 and Vulkan compile as separate static backend targets. `SparkleRHI` exposes the common contract and privately selects the enabled backend targets.

## Ownership

| Responsibility | Owner |
| --- | --- |
| Scene/view/material policy, passes, feature selection, history | Renderer |
| Pass order, resource lifetime intervals, alias plan, barriers, queue dependencies | Renderer frame graph |
| Explicit resources, descriptors, binding layouts, pipelines, command recording, submission tokens | Public RHI contract |
| Native resource creation, descriptor encoding, command emission, queues, swap chain, validation | Selected backend-private RHI target |
| DLSS/Streamline feature policy and resource tagging | Renderer external-provider target |
| Narrow native handles required by an external provider | RHI interop service |

Sparkle deliberately differs from NVRHI's optional automatic barrier tracking: the Renderer frame graph is the only high-level barrier and scheduling authority. RHI command lists remain explicit and do not infer a second dependency graph.

## Lifetime and Recording Rules

- Public resources use opaque generational handles; backend objects never escape through ordinary Renderer APIs.
- Destruction/reuse is retired by `RhiSubmissionToken`. Routine frame execution, resize, capture, and scene changes do not wait for device idle.
- A recording worker owns one move-only `RhiCommandRecordingLease`. It may record commands and use preassigned transient upload/descriptor storage; it does not submit, present, wait, or mutate global caches.
- Submission and presentation remain owned by the render coordinator.

This follows the NVRHI principles of safe deferred destruction, explicit command-list ownership, parallel recording, multi-queue submission, and narrow native escape hatches while retaining Sparkle's existing handle model.

## Backend Parity Contract

Parity means that both backends implement the same public operations and report unsupported hardware or integrations through capabilities. It does not mean manufacturing support that the API, driver, or integrated SDK does not provide.

The paired contract covers:

- resource creation, upload, readback, aliasing, and deferred release;
- descriptors, resource views, samplers, binding layouts, graphics/compute pipelines;
- graphics, compute, and copy recording/submission;
- explicit transitions, UAV barriers, draw, dispatch, and copy;
- classic BLAS/TLAS and capability-gated partitioned TLAS;
- presentation, capture, timestamps, debug markers/messages, and memory diagnostics;
- native device, queue, command-list, and resource interop through neutral handles.

Backend differences must be visible in `RhiCapabilities`. Current intentional differences include descriptor tables versus descriptor sets, feature level versus API version, queue topology, validation facilities, memory allocator, ray-tracing extensions, and external-provider bridge availability. Renderer-wide policy branches on those neutral facts; only a dedicated provider adapter may branch on `ERhiBackendApi`.

## NVIDIA Reference Mapping

NVRHI is the naming and responsibility reference, not a source template:

- NVRHI's interface/common/backend target split maps to `SparkleRHI`, `SparkleRHICommon`, `SparkleRHI_D3D12`, and `SparkleRHI_Vulkan`.
- NVRHI `IDevice`, `ICommandList`, binding, pipeline, query, and native-object responsibilities map to focused Sparkle `Rhi*Service` contracts and `RenderCommandList`.
- Donut's render-pass layer maps to Sparkle Renderer; it does not move into RHI.
- Donut device managers are a reference for backend bootstrap and presentation ownership. Sparkle currently composes those operations behind `RenderDeviceServices` because it is an engine module, while keeping every native implementation backend-private.

## Enforcement

`architecture_boundary_check` rejects Renderer-to-native/RHI-private dependencies, RHI-to-Renderer dependencies, Renderer feature policy in RHI, common-RHI native API usage, and cross-backend references. RHI CMake configuration also rejects backend source files assigned to the common, diagnostics, facade, or opposite backend target.

Acceptance requires three builds: combined D3D12+Vulkan, D3D12-only, and Vulkan-only. A compile proves contract and link separation; behavioral parity still requires paired runtime validation/capture on supported hardware.
