# RHI Device Lifecycle And Failure Recovery

Status: current feature dossier; source-backed, not device-run, fault-injection, recovery, or release evidence

Verified: 2026-09-07 at committed `master` revision `c28b33bd`

Scope: `RHI-LIFE-01` through `RHI-LIFE-06`; service composition, owner-thread access, publication, steady-state frame use, swapchain recovery, shutdown settlement, device loss, diagnostics, and the explicit absence of in-process device recreation

## Feature Promise And Non-Promise

A successful RHI creation request publishes one complete backend service composition to its owner thread. Its device, queues, presentation, resources, descriptors, pipelines, diagnostics, interop, capture, and UI services remain valid through frame execution and settle before reverse-order destruction.

The current implementation **does not promise in-process recovery from a removed/lost GPU device**. Swapchain resize/out-of-date handling and orderly shutdown are recoverable lifecycle transitions; D3D12 device removal and Vulkan `VK_ERROR_DEVICE_LOST` are diagnostic/fatal boundaries in the inspected path. Crash evidence is not a replacement device.

## Why This Is An Independent Contract

Backend selection answers *which* device may be created. Commands answer how work is recorded/submitted. Presentation answers how one swapchain is acquired and rebuilt. None owns the aggregate lifetime of every service or the transition from partial construction to published/active/settled/destroyed. Without this contract, a successful resize can be misreported as device recovery and a DRED or Vulkan error string can be misreported as a usable post-loss state.

## Current State Machine

```text
Unpublished
  -> validate request, compiled backend, window/config and interposer hooks
  -> construct backend device and dependent services
  -> Published / Owner-thread active
       -> Begin / record / submit / present frames
       -> Resize drain -> rebuild swapchain-dependent state -> Active
       -> Shutdown requested -> settle queues and recording views -> Settled
  -> destroy dependents before native device -> Destroyed

Any required creation failure -> no published services / fatal boundary
Device removed or VK_ERROR_DEVICE_LOST -> diagnostics + fatal boundary
                                      -> no current Recreating/Recovered state
```

`RenderDeviceServicesState` owns the concrete service object and enforces a `Threading::OwnerThread`. The public facade is non-copyable and non-movable, so the active composition has one stable aggregate owner. Backend constructors build the native device and the services that depend on it; destructors release those dependents before the native device.

## Lifecycle Matrix

| Transition | D3D12 current behavior | Vulkan current behavior | Required boundary |
| --- | --- | --- | --- |
| selection/create | create requested compiled backend, device, swapchain/presentation and service aggregate | create requested compiled backend, Vulkan 1.3 device, swapchain/presentation and service aggregate | failure before aggregate publication; no alternate backend silently activates |
| steady frame | begin, acquire/use back buffer, record/submit, present, advance frame slot | begin/acquire, record/submit, present, advance frame slot | owner-thread/frame identity and command contracts remain authoritative |
| resize/out-of-date | drain presentation work then resize swapchain | drain graphics/presentation work, recreate swapchain and dependent views; acquire may retry the resize case | this is presentation-resource recovery, not device recreation |
| orderly settle | wait RHI idle and clear allocator recording read view | clear acquire state, wait device idle, and settle owned services | no new frame work after settlement begins; queued users complete or fail explicitly |
| device loss | DRED/device-removed reason can be recorded | `VK_ERROR_DEVICE_LOST` is named by result handling | no inspected transition rebuilds device, services, graph resources, providers, or UI bindings |
| destruction | release dependent services/queues/presentation before device owner | release dependent services/queues/presentation before device owner | no service or native handle survives aggregate destruction |

## Publication, Concurrency, And Lifetime Rules

- Creation must be atomic at the public boundary: callers either receive a complete `RenderDeviceServices` composition or no usable facade.
- All direct service use belongs to the recorded owner thread. Renderer may prepare work elsewhere, but it reaches native services through the owned recording/submission contracts.
- A frame cannot span device generations because the current system has no device-generation/recreation model. If recovery is later implemented, device generation must enter resources, pipelines, descriptors, provider interop, viewport products, UI texture handles, captures, and graph/history invalidation.
- `SettleForShutdown` is a one-way coordination boundary, not a general-purpose per-frame wait. It must not fabricate queue completion after submission/device failure.
- Presentation resize may replace swapchain images/views after a drain while leaving the device aggregate active. Consumers must not retain old back-buffer identity.

## Failure And Observability Contract

| Failure class | Current safe claim | Evidence that must be retained |
| --- | --- | --- |
| invalid/uncompiled backend, invalid window/config, unsupported interposer path | creation rejects/fails before published use | requested backend/config, compiled target set, exact rejecting boundary |
| partial native or dependent-service creation | no complete public composition is returned | failing native call/service name and cleanup/leak record |
| swapchain out-of-date/minimize/resize | bounded drain/recreate/refusal path; device remains the same | old/new extent, image generation, acquire/present result, queue tokens |
| submission/wait/idle failure | no false completion or safe-retirement claim | queue, token, native result, last known frame and diagnostic markers |
| D3D12 removal | DRED/device-removed diagnostics where enabled; process path remains terminal | removed reason, breadcrumbs/page-fault data when available, adapter/driver/build identity |
| Vulkan device loss | named Vulkan result and diagnostic context; process path remains terminal | `VkResult`, queue/operation/frame context, validation messages, adapter/driver/build identity |

## Acceptance Criteria

- `AC-RHI-LIFE-01` — every valid creation publishes exactly one complete backend service aggregate on its owner thread; every invalid or injected partial-create case publishes none and releases all completed subobjects.
- `AC-RHI-LIFE-02` — service initialization and reverse destruction order preserve dependencies on both backends with no use-before-publication, use-after-destroy, live native object, or interop callback after shutdown starts.
- `AC-RHI-LIFE-03` — steady frame operations and shutdown settlement obey one legal lifecycle; new work is refused after settlement begins, all real completion is observed, and shutdown reaches a declared bound without fabricated tokens.
- `AC-RHI-LIFE-04` — resize/minimize/out-of-date transitions replace only swapchain-dependent identity after the required drain, invalidate all consumers of old images/views, and resume or remain paused without being reported as device recovery.
- `AC-RHI-LIFE-05` — injected D3D12 removal and Vulkan device loss produce attributable backend diagnostics and a terminal current-state result; no caller continues using the invalid facade or reports recovery.
- `AC-RHI-LIFE-06` — any future recovery proposal remains unimplemented until it owns device generation, aggregate recreation, producer/consumer invalidation, provider/native-handle rebinding, retained-work policy, failure bounds, and both-backend evidence.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-RHI-LIFE-01` | fail each construction stage or request an unavailable backend | no facade publication; already-created dependents release in valid order | `CHK-RHI-LIFE-01` |
| `FM-RHI-LIFE-02` | wrong-thread service use or work admitted while settling | owner/lifecycle guard rejects before native work | `CHK-RHI-LIFE-02` |
| `FM-RHI-LIFE-03` | resize/out-of-date/minimize during in-flight presentation | drain and replace only dependent identity; no stale view/back buffer is reused | `CHK-RHI-LIFE-03` |
| `FM-RHI-LIFE-04` | queue wait/idle never completes or returns native failure | no false completion; bounded failure record identifies queue/token/operation | `CHK-RHI-LIFE-02`, `CHK-RHI-LIFE-04` |
| `FM-RHI-LIFE-05` | D3D12 removal or Vulkan device loss | capture available diagnostics, stop using the aggregate, and terminate/refuse according to current policy | `CHK-RHI-LIFE-04` |
| `FM-RHI-LIFE-06` | code or docs imply automatic device recovery | source/capability audit fails until a real recreation owner and evidence exist | `CHK-RHI-LIFE-05` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-RHI-LIFE-01` | backend construction fault matrix with live-object/allocation cleanup inspection at every stage | `AC-RHI-LIFE-01`, `AC-RHI-LIFE-02`; `FM-RHI-LIFE-01` |
| `CHK-RHI-LIFE-02` | owner-thread/lifecycle state exercise around ordinary frame work, queued controls, settlement, repeated shutdown, and forced wait failure | `AC-RHI-LIFE-02`, `AC-RHI-LIFE-03`; `FM-RHI-LIFE-02`, `FM-RHI-LIFE-04` |
| `CHK-RHI-LIFE-03` | repeated resize/minimize/out-of-date loop with in-flight frames, generation-qualified back-buffer/view assertions, and validation | `AC-RHI-LIFE-04`; `FM-RHI-LIFE-03` |
| `CHK-RHI-LIFE-04` | backend-specific device-loss injection capturing DRED or Vulkan result/validation context and observing all post-loss callers | `AC-RHI-LIFE-05`; `FM-RHI-LIFE-04`, `FM-RHI-LIFE-05` |
| `CHK-RHI-LIFE-05` | enumerate lifecycle states, recovery APIs, device generations, and every native-handle consumer; assert no current route claims recreation | `AC-RHI-LIFE-06`; `FM-RHI-LIFE-06` |

This contract is **defined but unproved**. Normal construction and shutdown do not prove partial-failure cleanup, bounded settlement, device-loss diagnostics, or recovery.

Primary evidence destination: `RHI-E16` in the [Capability Evidence Plan](../../../../../../Plans/CapabilityEvidence.md#rhi-evidence).

## Primary Source Routes

- [`RenderDeviceServices.h`](../../../../../../../Engine/RHI/Public/Device/RenderDeviceServices.h) and common `Engine/RHI/Private/Device`
- `Engine/RHI/Private/D3D12/Device/D3D12RenderDeviceServices.cpp` and `Engine/RHI/Private/Vulkan/Device/VulkanRenderDeviceServices.cpp`
- D3D12/Vulkan command queues, swapchains, allocators, diagnostics, interop, and UI services whose lifetime is aggregated by the device service
- [Backend Selection and Device Capabilities](BackendSelectionAndDeviceCapabilities.md), [Command Submission and Synchronization](../PipelineAndExecution/CommandSubmissionAndSynchronization.md), [Presentation](../PresentationAndInterop/Presentation.md), and [Diagnostics](../DiagnosticsAndCapture/Diagnostics.md)
