# Renderer Latency Coordination

Status: current capability-gated feature dossier; source-backed, not end-to-end latency, pacing, or release evidence

Verified: 2026-09-06 through committed `master` revision `c28b33bd`; public facade, external runtime, Streamline, D3D12 frame, RHI hook, and build membership inspected; executable source is unchanged from the earlier `8414b5dc` audit

Scope: `REN-LAT-01` through `REN-LAT-05`; owns application simulation markers, optional Streamline PCL/Reflex coordination, and the Renderer-to-RHI marker identity that joins simulation, render submission, and presentation

## Feature Contract

Sparkle exposes `Renderer::BeginSimulationFrame(frameId)` and `EndSimulationFrame(frameId)` so host simulation can share one logical frame identity with render-submit and present markers. When the optional NVIDIA Streamline runtime is built, initialized, device-bound, presentation-ready, and PCL-supported, the six markers become Streamline PCL events. Reflex sleep runs only at SimulationStart when Reflex is supported.

```text
host BeginSimulationFrame(id) -> SimulationStart (+ optional Reflex sleep)
host EndSimulationFrame(id)   -> SimulationEnd
RHI BeginFrame(id)            -> RenderSubmitStart
RHI SubmitFrame(id)           -> RenderSubmitEnd -> PresentStart -> PresentEnd
```

This improves frame attribution and enables a provider to coordinate CPU simulation timing with the actual submitted/presented frame. It does not synthesize frames, choose rendering quality, guarantee lower latency, or constitute a general frame pacer.

## Ownership And Readiness

| Owner | Responsibility | Lifetime/thread boundary |
| --- | --- | --- |
| host/application | bracket simulation with the same monotonic logical frame ID later submitted to rendering | caller contract; sequencing is not repaired by Renderer |
| `RendererExternalRuntime` | initialize/shutdown shared external runtime and forward simulation markers | application-owner thread; asserts owner access |
| `StreamlineRuntime` | serialize initialization/shutdown, lease active calls, bind native device, detect PCL/Reflex support, create frame tokens, sleep, and emit PCL markers | process-shared static state guarded by mutex/condition variable |
| D3D12 RHI/device services | forward render-submit/present markers through interposer hooks around actual frame operations | D3D12 active-interposer path only in inspected source |
| build configuration | compile Streamline provider target with `SPARKLE_WITH_NVIDIA_STREAMLINE=1` only when option and target are available | otherwise calls compile to explicit no-op behavior |

Runtime readiness requires initialization, device binding, and presentation readiness. Calls made while shutting down or before the required readiness acquire no call lease and return without provider work. PCL support gates all marker emission; Reflex support independently gates only the sleep call.

## Current Support Matrix

| Cell | Current behavior | Claim boundary |
| --- | --- | --- |
| D3D12 + Streamline built + PCL supported | all six marker kinds route to PCL using one frame token identity | exact ordering and measured latency remain unproved |
| same + Reflex supported | SimulationStart first invokes `slReflexSleep`, then emits PCL SimulationStart | default `sl::ReflexOptions{}` only; no public mode/settings surface was found |
| D3D12 without Streamline/runtime/readiness/PCL | public calls and RHI hook path safely do no provider work | no latency feature result may be advertised |
| Vulkan | external runtime rejects Streamline initialization and no equivalent inspected Vulkan marker producer exists | unsupported/no-op, not backend parity |

The public frame ID is 64-bit but the Streamline frame-token request currently casts it to 32-bit. Wrap/collision behavior beyond `UINT32_MAX` logical frames has not been established and must be treated as an explicit limit.

## Ordering, Failure, And Non-Claims

- Simulation markers are host-driven; Renderer cannot guarantee that callers bracket simulation exactly once or use the same ID as the admitted render submission.
- RenderSubmitStart is emitted after the D3D12 presentation-slot wait and before frame-index/recording setup. RenderSubmitEnd follows the final graphics submission; PresentStart/End bracket the swapchain present call.
- Unknown marker vocabulary, frame-token acquisition failure, Reflex sleep failure, PCL marker rejection, and initial Reflex option rejection are fatal in the active provider path.
- Initialization or unsupported readiness returns an empty interposer/no-op route rather than failing the renderer. This is capability absence, not successful PCL/Reflex activation.
- Shutdown stops new leases and waits for active provider calls before `slShutdown`; deadlock/bounded shutdown still needs stress evidence.
- No selector, requested/active latency diagnostic, user-facing Reflex mode, marker counter, or measured latency product was found. SDK registration alone must not be presented as enabled low-latency mode.
- This contract is independent from [Frame Generation](../PostProcessing/ReconstructionAndGeneration/FrameGeneration.md), which remains absent.

## Acceptance Criteria

- `AC-LAT-01` — one admitted logical frame uses the same identity and exactly ordered SimulationStart, SimulationEnd, RenderSubmitStart, RenderSubmitEnd, PresentStart, and PresentEnd markers at their documented boundaries.
- `AC-LAT-02` — host misuse (duplicate, omitted, reordered, or mismatched simulation ID) is detectable and cannot be reported as a valid complete latency trace.
- `AC-LAT-03` — active D3D12 PCL maps every marker to the correct Streamline enum/token; Reflex sleep occurs exactly once at SimulationStart only when supported.
- `AC-LAT-04` — absent build support, unsupported backend/adapter/features, incomplete readiness, and shutdown yield the documented no-op/unavailable state without claiming activation.
- `AC-LAT-05` — token/sleep/marker failure is explicit and cannot leave the shared runtime partially active; shutdown waits for active calls and completes exactly once.
- `AC-LAT-06` — 64-to-32-bit frame identity behavior is proven across the declared supported run length or the supported limit is enforced before wrap.
- `AC-LAT-07` — any advertised latency benefit is backed by an end-to-end measurement workload and configuration, not marker presence.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-LAT-01` | duplicate/missing/reordered/mismatched host simulation marker | trace is incomplete/invalid and identifies the logical frame defect | `CHK-LAT-01` |
| `FM-LAT-02` | Streamline absent, non-D3D12 backend, unsupported PCL/Reflex, or presentation not ready | explicit unavailable/no-op state; render/present proceeds without feature claim | `CHK-LAT-02` |
| `FM-LAT-03` | token acquisition, Reflex sleep, option, or PCL marker call fails | active route fails visibly and does not emit a fabricated success result | `CHK-LAT-03` |
| `FM-LAT-04` | shutdown races a marker/provider call | no new lease enters; active calls drain; runtime shuts down once without deadlock/use-after-shutdown | `CHK-LAT-03` |
| `FM-LAT-05` | frame ID reaches/crosses 32-bit wrap | enforced limit or demonstrated collision-safe provider identity; no silent wrong-frame attribution | `CHK-LAT-04` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-LAT-01` | instrument one host/simulation/render/present sequence plus duplicate/omitted/reordered/mismatched cases; compare exact ID/order/count | `AC-LAT-01`, `AC-LAT-02`; `FM-LAT-01` |
| `CHK-LAT-02` | build/runtime matrix for Streamline on/off, D3D12/Vulkan, supported/unsupported PCL/Reflex, and readiness transitions; inspect active/unavailable result | `AC-LAT-03`, `AC-LAT-04`; `FM-LAT-02` |
| `CHK-LAT-03` | provider fault injection at initialization, token, option, sleep, marker, active-call shutdown, and repeated shutdown | `AC-LAT-05`; `FM-LAT-03`, `FM-LAT-04` |
| `CHK-LAT-04` | direct token-identity test at `UINT32_MAX-1`, `UINT32_MAX`, and next 64-bit IDs | `AC-LAT-06`; `FM-LAT-05` |
| `CHK-LAT-05` | only after correctness: capture end-to-end latency under declared baseline/Reflex configurations with matched workload and statistics | `AC-LAT-07` |

This contract is **defined but unproved**. Current source proves a capability-gated marker/sleep route, not correct host bracketing, Vulkan support, a configurable Reflex product, or any latency reduction.

## Primary Source Routes

- [`Renderer.h`](../../../../../../../Engine/Renderer/Public/Renderer.h) and [`Renderer.cpp`](../../../../../../../Engine/Renderer/Private/Renderer.cpp)
- [`RendererExternalRuntime.cpp`](../../../../../../../Engine/Renderer/Private/Integrations/RendererExternalRuntime.cpp)
- [`StreamlineRuntimeSupport.cpp`](../../../../../../../Engine/Renderer/Private/Streamline/StreamlineRuntimeSupport.cpp)
- [`RhiFrameLatencyMarker.h`](../../../../../../../Engine/RHI/Public/Presentation/RhiFrameLatencyMarker.h) and [`RhiInterposerHooks.h`](../../../../../../../Engine/RHI/Public/Interop/RhiInterposerHooks.h)
- [`D3D12RenderDeviceServices.cpp`](../../../../../../../Engine/RHI/Private/D3D12/Device/D3D12RenderDeviceServices.cpp)
- [`CMakeLists.txt`](../../../../../../../Engine/Renderer/CMakeLists.txt)
