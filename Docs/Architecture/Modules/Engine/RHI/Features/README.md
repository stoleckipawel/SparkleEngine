# RHI Feature Dossiers

Status: RHI feature index; routes current source claims and local proof contracts, not backend or release approval

Scope: map every public RHI contract and backend implementation family to a cohesive owner without turning the capability inventory into a mixed architecture manual

Authority boundary: the [Capability Inventory](../CapabilityInventory.md) owns exact `RHI-*` state and evidence rows. These dossiers own mechanism, ownership, lifetime, failure, and feature-local completion contracts. Renderer owns feature policy; Acceptance owns candidate results and release disposition.

## Dossier Contract

Each dossier identifies its promise and non-promise, neutral owner, D3D12/Vulkan lowering boundary, inputs and outputs, mutable state and lifetime, requested-versus-active behavior when applicable, capacity/failure behavior, source routes, acceptance criteria, controlled failures, checks, and completion definition. Unknown executable behavior remains unproved and routes to the [Capability Evidence Plan](../../../../../Plans/CapabilityEvidence.md).

## Feature Map

| Capability families | Dossier | Independent result |
| --- | --- | --- |
| `RHI-BACK-*`, `RHI-DEV-*` | [Backend Selection and Device Capabilities](BackendSelectionAndDeviceCapabilities.md) | one requested compiled backend becomes a device with truthful neutral capabilities and queue topology |
| `RHI-RES-*`, `RHI-FMT-*` | [Resource Lifetime and Memory](ResourceLifetimeAndMemory.md) | neutral resource intent becomes allocated, usable, observable, and completion-safely retired GPU storage |
| `RHI-BIND-*` | [Descriptor Binding](DescriptorBinding.md) | a validated layout and writes become backend descriptors whose identity survives recording and execution |
| `RHI-PIPE-*` plus shader/reflection contracts | [Pipeline and Shader Contracts](PipelineAndShaderContracts.md) | complete neutral shader and fixed-function identity becomes one validated backend pipeline |
| `RHI-CMD-*` | [Command Submission and Synchronization](CommandSubmissionAndSynchronization.md) | recorded operations become ordered queue submissions with explicit waits and completion authority |
| `RHI-RT-*`, `RHI-RTC-*` | [Ray Tracing](RayTracing.md) | neutral AS/traversal/SBT requests become capability-gated native ray work |
| `RHI-PRES-*` | [Presentation](Presentation.md) | acquired back-buffer work becomes a paced, resized, presented window image |
| `RHI-DIAG-01` through `RHI-DIAG-05` | [Diagnostics](Diagnostics.md) | native observations become attributable, bounded diagnostic facts |
| `RHI-DIAG-06` | [Texture Capture](TextureCapture.md) | one supported texture becomes an asynchronous readback result or explicit failure |
| `RHI-DIAG-07` | [External Interop](ExternalInterop.md) | deliberately narrow native access supports an eligible external provider without leaking into the neutral contract |
| `RHI-DIAG-08` | [ImGui Rendering](ImGuiRendering.md) | immutable UI draw data and texture identity become backend commands with completion-safe lifetime |

## Source-Owner Coverage Audit

This table proves documentation reachability over the complete current `Engine/RHI` source shape; it does not replace the exact capability ledger.

| Current source owner | Documentation owner | Coverage decision |
| --- | --- | --- |
| `Public/RHIAPI.h`, `Public/Core`, `Public/Device`, `Public/Config`, `Public/CVars`; common `Private/Core`, `Private/Device`, `Private/Config`, `Private/CVars` | [Backend Selection and Device Capabilities](BackendSelectionAndDeviceCapabilities.md) | export boundary, backend identity, facade/service composition, capability truth, defaults, and device/queue selection |
| `Public/Formats`, `Public/Resources`, `Public/Textures`, `Public/Samplers`, `Public/Memory`; common `Private/Formats`, `Private/Resources`, `Private/Memory`; matching backend folders | [Resource Lifetime and Memory](ResourceLifetimeAndMemory.md) | formats, resource/view descriptions, upload/readback support, allocation, budgets, recording use, aliasing, and retirement |
| `Public/Bindings`, `Public/Descriptors`; common `Private/Bindings`, `Private/Descriptors`; matching backend folders | [Descriptor Binding](DescriptorBinding.md) | layout/set/table identity, allocation, writes, arrays, capability gating, and recording pools/heaps |
| `Public/Pipeline`, `Public/Shaders`, `Public/ShaderParameters`; common `Private/Pipeline`, `Private/Shaders`, `Private/ShaderParameters`, `Private/Validation`; matching backend pipeline folders | [Pipeline and Shader Contracts](PipelineAndShaderContracts.md) | bytecode/reflection, parameter ABI, descriptor validation, fixed-function state, pipeline lowering, and cache identity |
| `Public/Commands`, `Public/Frame`; common `Private/Commands` and backend command/recording-use tables | [Command Submission and Synchronization](CommandSubmissionAndSynchronization.md) | leases, lists, queue topology, barriers, submits, waits, tokens, frame identity, and completion-safe use |
| `Public/RayTracing`; common `Private/RayTracing` and matching backend folders | [Ray Tracing](RayTracing.md) | BLAS/TLAS/PTLAS, inline/native traversal, pipeline/SBT contracts, transforms, capability checks, and native providers |
| `Public/Presentation`; common `Private/Presentation` and backend `SwapChain` folders | [Presentation](Presentation.md) | defaults, frame-latency markers, swapchain, acquire, resize, state, pacing, VSync, and present |
| `Public/Diagnostics`; common `Private/Diagnostics` and matching backend folders | [Diagnostics](Diagnostics.md) | labels, events, timestamps, validation/crash data, live objects, delivery bounds, and observer configuration |
| `Public/Capture`; common `Private/Capture` and matching backend folders | [Texture Capture](TextureCapture.md) | asynchronous readback, staging lifetime, format/layout conversion, polling, result delivery, failure, and cleanup |
| `Public/Interop`; common `Private/Interop`, backend interop, external-feature capability, and D3D12 interposer seams | [External Interop](ExternalInterop.md) | native handles/state/hooks, named provider constraints, generation, fallback, and package boundary |
| `Public/UI`; backend `UI` | [ImGui Rendering](ImGuiRendering.md) | backend device objects, font/texture descriptors, draw lowering, clipping, resize, and completion lifetime |
| `Private/D3D12`, `Private/Vulkan`, and `Engine/RHI/CMakeLists.txt` | every dossier plus [Capability Inventory](../CapabilityInventory.md) | each neutral family is checked across both lowerings; build membership and optional gates remain executable CMake authority |

No public or private RHI directory is an undocumented catch-all. PCH, include, native conversion, and third-party helper files belong to the contract they enable and do not form independent capabilities.

## Documentation Contract Coverage

| Feature owner | Contract-definition state | Evidence boundary |
| --- | --- | --- |
| Backend/device | Defined, unproved | build both target shapes and exercise unavailable/default/requested backend, adapter, queue, and capability reporting cases |
| Resources/memory | Defined, unproved | exercise format/use matrices, allocation/alias/upload/readback, pressure, in-flight lifetime, and both allocators |
| Descriptors/bindings | Defined, unproved | exercise layouts/writes/arrays/capacity, invalid requests, recording lifetime, and backend equivalence |
| Pipelines/shaders | Defined, unproved | exercise complete descriptor/ABI identity, invalid combinations, generation retirement, and both lowerings |
| Commands/synchronization | Defined, unproved | exercise queue dependencies, barriers, aliasing, lease misuse, failure, stall, and shutdown |
| Ray tracing | Defined, unproved | exercise AS/SBT/traversal matrices, unsupported combinations, vendor gates, semantic parity, and retirement |
| Presentation | Defined, unproved | exercise acquire/resize/minimize/VSync/pacing/device-loss flows and explicit HDR rejection on both backends |
| Diagnostics | Defined, unproved | correlate engine/native identity, faults, timestamps, delivery bounds, observer cost, and backend-specific availability |
| Texture capture | Defined, unproved | exercise pattern decoding, formats/layout, asynchronous states, faults, bounds, resize/shutdown, and both backends |
| External interop | Defined, unproved | exercise capability gates, native identity/state, manual/interposer routes, fallback, generation lifetime, and packaging |
| ImGui rendering | Defined, unproved | exercise draw order, clipping, texture/blend/color semantics, descriptor lifetime, resize/shutdown, and both backends |

`Defined` means the local contract is documented; it does not mean any criterion passed. Candidate results belong in Acceptance reports.

## Placement And Granularity

- A dossier follows one neutral contract family through common code and both backends; backend-specific copies would duplicate authority.
- Resources and memory remain together because allocation, recording use, aliasing, and GPU-completion retirement enforce one lifetime invariant. Descriptors remain separate because their layout/write/capacity contract changes independently.
- Pipelines and shader contracts remain together because reflection and parameter layout define the pipeline ABI. Command submission stays separate because queue order and completion own execution lifetime.
- Diagnostics, texture capture, external interop, and ImGui rendering remain separate: they have different callers, products, failure states, lifecycle triggers, evidence oracles, and package risks even though all touch backend-native services.
- Exact capability rows stay in [Capability Inventory](../CapabilityInventory.md); these pages link rather than copy row tables.
